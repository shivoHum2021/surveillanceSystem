#include "TVMRunner.hpp"
#include <iterator>
using namespace ::tvm::runtime;

namespace camera
{
    namespace camera_ml
    {
        TVMRunner::TVMRunner(const std::string &path, const std::string &device): ModelProcessor(path, device)
        {
            isRunWasCalled =false; 
        }

        int TVMRunner::initializeModelInterface()
        {
            int ret = 1;

            if (0 == Load())
            {
                GetMetaInfo();
                ret = 0;
            }
            return ret;
        }

        std::vector<DetectionOutput> TVMRunner::runModelInterface(char *inputFrame)
        {
            LOG(INFO) << "TVMRunner :runModelInterface:";
            std::string input_id = "normalized_input_image_tensor";
            NDArray in_arr = mGraphHandle.GetFunction("get_input")(input_id);
            auto ssize = GetMemSize(in_arr);
            in_arr.CopyFromBytes(inputFrame, ssize);
            Run();
            std::vector<DetectionOutput> output;
            return output;
        }
        DLDeviceType TVMRunner::GetTVMDevice(std::string device)
        {
            if (!device.compare("cpu"))
            {
                return kDLCPU;
            }
            else
            {
                LOG(FATAL) << "TVMRunner : Unsupported device :" << device;
            }
        }
        size_t TVMRunner::GetMemSize(tvm::runtime::NDArray &narr)
        {
            size_t size = 1;
            for (tvm_index_t i = 0; i < narr->ndim; ++i)
            {
                size *= static_cast<size_t>(narr->shape[i]);
            }
            size *= (narr->dtype.bits * narr->dtype.lanes + 7) / 8;
            return size;
        }
        int TVMRunner::Load(void)
        {
            LOG(INFO) << "TVMRunner Load:" << mModelPath;
            // Load the lib file
            auto tstart = std::chrono::high_resolution_clock::now();
            mModHandle = Module::LoadFromFile((mModelPath + "/mod.so").c_str(), "so");
            auto tend = std::chrono::high_resolution_clock::now();
            r_module_load_ms = static_cast<double>((tend - tstart).count()) / 1e6;

            tstart = std::chrono::high_resolution_clock::now();
            // Read model json file
            std::ifstream json_reader((mModelPath + "/mod.json").c_str());
            CHECK(!json_reader.fail()) << "Failed to open json file:" << (mModelPath + "/mod.json").c_str();
            json_reader.seekg(0, std::ios_base::end);
            std::size_t json_size = json_reader.tellg();
            json_reader.seekg(0, std::ios_base::beg);
            std::string json_data;
            json_data.reserve(json_size);
            json_reader.read((char *)json_data.c_str(), json_size);
            json_reader.close();

            // Get ref to graph exeutor
            auto f_handle = tvm::runtime::Registry::Get("tvm.graph_executor.create");

            // Greate graph runtime
            mGraphHandle = (*f_handle)(json_data, mModHandle, static_cast<int>(GetTVMDevice(mDevice)), 0);
            tend = std::chrono::high_resolution_clock::now();
            r_graph_load_ms = static_cast<double>((tend - tstart).count()) / 1e6;

            // Read params binary file
            tstart = std::chrono::high_resolution_clock::now();
            std::ifstream params_reader((mModelPath + "/mod.params").c_str(), std::ios::binary);
            CHECK(!params_reader.fail()) << "Failed to open json file:" << (mModelPath + "/mod.params").c_str();
            params_reader.seekg(0, std::ios_base::end);
            std::size_t param_size = params_reader.tellg();
            params_reader.seekg(0, std::ios_base::beg);
            std::vector<char> param_data(param_size / sizeof(char));
            params_reader.read((char *)&param_data[0], param_size);
            params_reader.close();
            TVMByteArray params_arr;
            params_arr.data = (char *)&param_data[0];
            params_arr.size = param_size;
            tend = std::chrono::high_resolution_clock::now();
            r_param_read_ms = static_cast<double>((tend - tstart).count()) / 1e6;

            // Load parameters
            tstart = std::chrono::high_resolution_clock::now();
            mGraphHandle.GetFunction("load_params")(params_arr);
            tend = std::chrono::high_resolution_clock::now();
            r_param_load_ms = static_cast<double>((tend - tstart).count()) / 1e6;
            return 0;
        }
        int TVMRunner::Run(void)
        {
            isRunWasCalled = true;
            mGraphHandle.GetFunction("run")();
            return 0;
        }
        /*!
         * \brief Query various metadata from the grsph runtime.
         * \param 0 on success else error code.
         */
        TVMMetaInfo TVMRunner::GetMetaInfo(void)
        {
            LOG(INFO) << "TVMRunner::GetMetaInfo";
            mInfo.n_inputs = mGraphHandle.GetFunction("get_num_inputs")();
            mInfo.n_outputs = mGraphHandle.GetFunction("get_num_outputs")();
            Map<String, ObjectRef> tvm_input_info = mGraphHandle.GetFunction("get_input_info")();
            auto shape_info = GetRef<Map<String, ObjectRef>>(tvm_input_info["shape"].as<MapNode>());
            auto dtype_info = GetRef<Map<String, ObjectRef>>(tvm_input_info["dtype"].as<MapNode>());
            for (const auto &kv : shape_info)
            {
                auto stuple = GetRef<ShapeTuple>(kv.second.as<ShapeTupleObj>());
                std::vector<int64_t> vshape;
                vshape.assign(stuple.begin(), stuple.end());
                auto dtype = GetRef<String>(dtype_info[kv.first].as<StringObj>());
                std::pair<std::vector<int64_t>, std::string> value = std::make_pair(vshape, dtype);
                mInfo.input_info.insert({kv.first, value});
            }

            tvm_input_info = mGraphHandle.GetFunction("get_output_info")();
            shape_info = GetRef<Map<String, ObjectRef>>(tvm_input_info["shape"].as<MapNode>());
            dtype_info = GetRef<Map<String, ObjectRef>>(tvm_input_info["dtype"].as<MapNode>());
            for (const auto &kv : shape_info)
            {
                auto stuple = GetRef<ShapeTuple>(kv.second.as<ShapeTupleObj>());
                std::vector<int64_t> vshape;
                vshape.assign(stuple.begin(), stuple.end());
                auto dtype = GetRef<String>(dtype_info[kv.first].as<StringObj>());
                std::pair<std::vector<int64_t>, std::string> value = std::make_pair(vshape, dtype);
                mInfo.output_info.insert({kv.first, value});
            }
            PrintMetaInfo();
            return mInfo;
        }
        void TVMRunner::PrintMetaInfo(void)
        {
            LOG(INFO) << "Meta Information:" << mModelPath;
            LOG(INFO) << "    Number of Inputs:" << mInfo.n_inputs;
            LOG(INFO) << "    Number of Outputs:" << mInfo.n_outputs;
            LOG(INFO) << "    Input MetaInfo:";
            for (auto &elem : mInfo.input_info)
            {
                std::ostringstream stream;
                stream << "[";
                copy(elem.second.first.begin(), elem.second.first.end() - 1,
                     std::ostream_iterator<int>(stream, ", "));
                stream << elem.second.first.back() << "]";
                LOG(INFO) << "        Input:" << elem.first;
                LOG(INFO) << "            DType:" << elem.second.second;
                LOG(INFO) << "            Shape:" << stream.str();
            }
            LOG(INFO) << "    Output MetaInfo:";
            int i = 1;
            for (auto &elem : mInfo.output_info)
            {
                LOG(INFO) << "Meta OK1";
                std::ostringstream stream;
                stream << "[";
                if (i < 4)
                {
                    copy(elem.second.first.begin(), elem.second.first.end() - 1,
                         std::ostream_iterator<int>(stream, ", "));
                    stream << elem.second.first.back();
                }
                stream << "]";
                LOG(INFO) << "Meta OK2";
                LOG(INFO) << "        Output:" << elem.first;
                LOG(INFO) << "            DType:" << elem.second.second;
                LOG(INFO) << "            Shape:" << stream.str();
                i++;
            }
            LOG(INFO) << "Done printing" << mModelPath;
            PrintStats();
        }
        void TVMRunner::PrintStats(void)
        {
            LOG(INFO) << "Performance Stats:" << mModelPath;
            LOG(INFO) << "    Module Load              :" << r_module_load_ms << " ms";
            LOG(INFO) << "    Graph Runtime Create     :" << r_graph_load_ms << " ms";
            LOG(INFO) << "    Params Read              :" << r_param_read_ms << " ms";
            LOG(INFO) << "    Params Set               :" << r_param_load_ms << " ms";
            LOG(INFO) << "Total Load Time     :" << r_module_load_ms + r_graph_load_ms + r_param_read_ms + r_param_load_ms << " ms";
        }
    } // camera::mp
} // camera
