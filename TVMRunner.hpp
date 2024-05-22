// TVMRunner.hpp
#ifndef TVM_RUNNER_HPP
#define TVM_RUNNER_HPP

#include "ModelProcessor.hpp"
#include <tvm/runtime/module.h>
#include <tvm/runtime/packed_func.h>
#include <tvm/runtime/registry.h>
#include "tvm/runtime/c_runtime_api.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <streambuf>
#include <string>
#include <vector>
namespace camera
{
    namespace camera_ml
    {
        typedef struct _TVMMetaInfo
        {
            int n_inputs;
            int n_outputs;
            std::map<std::string, std::pair<std::vector<int64_t>, std::string>> input_info;
            std::map<std::string, std::pair<std::vector<int64_t>, std::string>> output_info;
        } TVMMetaInfo;

        class TVMRunner : public ModelProcessor
        {
        public:
            TVMRunner(const std::string& path, const std::string& device);
            int initializeModelInterface() override;
            std::vector<float> runModelInterface(char *inputFrame) override;

        private:
            int Load(void);
            /*! \brief Executes one inference cycle */
            int Run(void);
            TVMMetaInfo GetMetaInfo();
            void PrintMetaInfo(void);
            void PrintStats(void);
            DLDeviceType GetTVMDevice(std::string device);
            size_t GetMemSize(tvm::runtime::NDArray &narr);
            bool isRunWasCalled;
            tvm::runtime::Module mModHandle;
            tvm::runtime::Module mGraphHandle;
            /*! \brief Holds meta information queried from graph runtime */
            TVMMetaInfo mInfo;
            int r_module_load_ms{0};
            /*! Graph runtime creatint time */
            int r_graph_load_ms{0};
            /*! Params read time */
            int r_param_read_ms{0};
            /*! Params load time */
            int r_param_load_ms{0};
        };

    }
}
#endif // TVM_RUNNER_HPP
