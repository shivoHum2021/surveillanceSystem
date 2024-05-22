#include "RTMessageBroker.hpp"
#include <sstream>
#include <chrono>
namespace camera
{
    namespace camera_ml
    {
        RTMessageBroker::RTMessageBroker(SurveillanceSystem *surveillance)
            : surveillanceRef(surveillance)
        {
            mTerm = false;
            // Constructor implementation (if needed)
        }
        RTMessageBroker::~RTMessageBroker()
        {
            rtConnection_Destroy(connectionSend);
            rtConnection_Destroy(connectionRecv);
        }

        int RTMessageBroker::rtMsgInit()
        {
            rtLog_SetLevel(RT_LOG_INFO);
            rtLog_SetOption(rdkLog);
            rtConnection_Create(&connectionSend, "SMART_TN_SEND", "tcp://127.0.0.1:10001");
            rtConnection_Create(&connectionRecv, "SMART_TN_RECV", "tcp://127.0.0.1:10001");
            rtConnection_AddListener(connectionRecv, "RDKC.SMARTTN.CAPTURE", onMsgCaptureFrame, this);
            rtConnection_AddListener(connectionRecv, "RDKC.SMARTTN.METADATA", onMsgProcessFrame, this);
            rtConnection_AddListener(connectionRecv, "RDKC.CVR.CLIP.STATUS", onMsgCvr, this);
            rtConnection_AddListener(connectionRecv, "RDKC.CVR.UPLOAD.STATUS", onMsgCvrUpload, this);
            return 0;
        }

        void RTMessageBroker::onMsgCaptureFrame(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure)
        {
            RTMessageBroker *self = static_cast<RTMessageBroker *>(closure);
            if (!self)
            {
                return; // Error handling if self is nullptr
            }
            rtMessage m;
            rtMessage_FromBytes(&m, buff, n);
            int processPID;
            char const *strFramePTS;
            rtMessage_GetInt32(m, "processID", &processPID);
            rtMessage_GetString(m, "timestamp", &strFramePTS);
            long long lResFramePTS;
            std::istringstream iss(strFramePTS);
            iss >> lResFramePTS;
            self->surveillanceRef->captureFrame(lResFramePTS);
            rtMessage_Release(m);
            return;
        }

        void RTMessageBroker::onMsgProcessFrame(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure)
        {
            RTMessageBroker *self = static_cast<RTMessageBroker *>(closure);
            if (!self)
            {
                return; // Error handling if self is nullptr
            }
            rtMessage m;
            rtMessage_FromBytes(&m, buff, n);
            MotionEventMetadata metaData;
            MotionEventMetadata::parseMessage(&metaData, m);
            int motionFlags;
            rtMessage_GetInt32(m, "motionFlags", &motionFlags);
            self->surveillanceRef->processFrameMetaData(metaData, motionFlags);
            rtMessage_Release(m);
            return;
        }
        void RTMessageBroker::onMsgCvr(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure)
        {
            RTMessageBroker *self = static_cast<RTMessageBroker *>(closure);
            if (!self)
            {
                return; // Error handling if self is nullptr
            }
            int clipGenStatus = -1;
            char const *cvrClipFname = NULL;
            uint64_t cvrEventTS = 0;
            const char *delimPos = 0;
            rtMessage req;
            rtMessage_FromBytes(&req, buff, n);
            rtMessage_GetInt32(req, "clipStatus", &clipGenStatus);
            rtMessage_GetString(req, "clipname", &cvrClipFname);
            LOG_DEBUG("clip status:" << clipGenStatus << " CVR clip name: " << cvrClipFname);
            // take action on clip gen status
            if (clipGenStatus == camera::camera_ml::CVR_CLIP_GEN_START)
            {
                self->surveillanceRef->OnClipGenStart(cvrClipFname);
            }
            else if (clipGenStatus == camera::camera_ml::CVR_CLIP_GEN_END)
            {
                self->surveillanceRef->OnClipGenEnd(cvrClipFname);
            }
            else
            {
                LOG_ERROR("Invalid Clip Gen status");
            }
            rtMessage_Release(req);
            return;
        }
        void RTMessageBroker::onMsgCvrUpload(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure)
        {
        }
        void RTMessageBroker::onMsgRefresh(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure)
        {
        }
        int RTMessageBroker::receiveRtmessage()
        {
            rtError err;
            while (mTerm)
            {
                err = rtConnection_Dispatch(connectionRecv);
                if (err != RT_OK)
                {
                    // rtLog_Debug("dispatch:%s", rtStrError(err));
                    LOG_INFO("dispatch:" << rtStrError(err));
                    LOG_INFO("Error receiving msg via rtmessage\n");
                }
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep to reduce CPU usage
            }
            LOG_INFO("Exit rtMessage listening loop .\n");
            return 0;
        }
        int RTMessageBroker::notify(const char *status)
        {
            if (!status)
            {
                LOG_ERROR("Trying to use invalid memory location!! ");
                return -1;
            }
            rtMessage req;
            rtMessage_Create(&req);
            rtMessage_SetString(req, "status", status);
            rtError err = rtConnection_SendMessage(connectionSend, req, "RDKC.SMARTTN.STATUS");
            rtLog_Debug("SendRequest:%s", rtStrError(err));
            if (err != RT_OK)
            {
                LOG_ERROR("Error sending msg via rtmessage\n");
            }
            rtMessage_Release(req);
            return 0;
        }
    }
}