#ifndef RTMESSAGEBROKER_HPP
#define RTMESSAGEBROKER_HPP

#include "SurveillanceSystem.hpp"
#include <rtMessage.h>
#include <rtConnection.h>
#include <rtLog.h>

namespace camera
{
    namespace camera_ml
    {
        class RTMessageBroker
        {
        private:
            rtConnection connectionSend;
            rtConnection connectionRecv;
            SurveillanceSystem *surveillanceRef; // Reference to Surveillance instance
            bool mTerm;
        public:
            RTMessageBroker(SurveillanceSystem *surveillance);
            ~RTMessageBroker();

            int rtMsgInit();
            int receiveRtmessage();
            int notify(const char* status);
            static void onMsgCaptureFrame(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
            static void onMsgProcessFrame(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
            static void onMsgCvr(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
            static void onMsgCvrUpload(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
            static void onMsgRefresh(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
        };
    }
}
#endif // RTMESSAGEBROKER_HPP
