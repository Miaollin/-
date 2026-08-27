#pragma once
#include <string>
#include <fstream>
#include <cstdio>
#include "types.h"
#include "macros.h"
#include "lf_queue.h"
#include "thread_utils.h"
#include "time_utils.h"

namespace Common{
    enum class LogType:int8_t{
        CHAR=0,
        INTEGER=1,
        LONG_INTEGER=2,
        LONG_LONG_INTEGER=3,
        UNSIGNED_INTEGER=4,
        UNSIGNED_LONG_INTEGER=5,
        UNSIGNED_LONG_LONG_INTEGER=6,
        FLOAT=7,
        DOUBLE=8
    };

    struct LogElement{
        LogType type_ = LogType::CHAR;
        union{
            char c;
            int i;
            long l;
            long long ll;
            unsigned u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;
        }u_;
    };

    class Logger final{
    private:
        const std::string file_name_;
        std::ofstream file_;
        LFQueue<LogElement> queue_;
        std::atomic<bool> running_ = {true};
        std::thread *logger_thread_=nullptr;
    public:
        explicit Logger(const std::string &file_name):file_name_(file_name),queue_(LOG_QUEUE_SIZE){
            file_.open(file_name);
            ASSERT(file_.is_open(),"Could not open log file:"+file_name);
            logger_thread_=createAndStartThread(-1,"Common/Logger",[this](){
                flushQueue();
            });
            ASSERT(logger_thread_ != nullptr, "Failed to start Logger thread.");
        }
        auto flushQueue() noexcept -> void {
            while (running_) {
                for (auto next = queue_.getNextToRead(); queue_.size() && next; next = queue_.getNextToRead()) {
                    switch (next->type_) {
                        case LogType::CHAR:
                            file_ << next->u_.c;
                            break;
                        case LogType::INTEGER:
                            file_ << next->u_.i;
                            break;
                        case LogType::LONG_INTEGER:
                            file_ << next->u_.l;
                            break;
                        case LogType::LONG_LONG_INTEGER:
                            file_ << next->u_.ll;
                            break;
                        case LogType::UNSIGNED_INTEGER:
                            file_ << next->u_.u;
                            break;
                        case LogType::UNSIGNED_LONG_INTEGER:
                            file_ << next->u_.ul;
                            break;
                        case LogType::UNSIGNED_LONG_LONG_INTEGER:
                            file_ << next->u_.ull;
                            break;
                        case LogType::FLOAT:
                            file_ << next->u_.f;
                            break;
                        case LogType::DOUBLE:
                            file_ << next->u_.d;
                            break;
                    }
                    queue_.updateReadIndex();
                }
                file_.flush();
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
                }
        }

        ~Logger(){
            std::cerr<<"Flushing and closing logger for "<<file_name_<<std::endl;
            while(queue_.size()){
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(1s);
            }
            running_ = false;
            logger_thread_->join();
            file_.close();
        }

        Logger() =delete;
        Logger(const Logger &)=delete;
        Logger(Logger &&)=delete;
        Logger &operator=(const Logger &)=delete;
        Logger &operator=(Logger &&)=delete;

        void pushValue(const LogElement& log_element) noexcept{
            *(queue_.getNextToWrite()) = log_element;
            queue_.updateWriteIndex();
        }

        void pushValue(const char value) noexcept{
            LogElement element{};
            element.type_ = LogType::CHAR;
            element.u_.c = value;
            pushValue(element);
        }

        void pushValue(const char *value) noexcept{
            while(*value){
                pushValue(*value);
                ++value;
            }
        }
        void pushValue(const std::string &value) noexcept{
            pushValue(value.c_str());
        }

        void pushValue(const int value) noexcept{
            LogElement element{};
            element.type_ = LogType::INTEGER;
            element.u_.i = value;
            pushValue(element);
        }
        void pushValue(const long value) noexcept{
            LogElement element{};
            element.type_ = LogType::LONG_INTEGER;
            element.u_.l = value;
            pushValue(element);
        }
        auto pushValue(const unsigned int value) noexcept -> void {
            LogElement element{};
            element.type_ = LogType::UNSIGNED_INTEGER;
            element.u_.u = value;
            pushValue(element);
        }
        auto pushValue(const unsigned long value) noexcept -> void {
            LogElement element{};
            element.type_ = LogType::UNSIGNED_LONG_INTEGER;
            element.u_.ul = value;
            pushValue(element);
        }

        auto pushValue(const unsigned long long value) noexcept -> void {
            LogElement element{};
            element.type_ = LogType::UNSIGNED_LONG_LONG_INTEGER;
            element.u_.ull = value;
            pushValue(element);
        }

        auto pushValue(const float value) noexcept {
            LogElement element{};
            element.type_ = LogType::FLOAT;
            element.u_.f = value;
            pushValue(element);
        }

        auto pushValue(const double value) noexcept {
            LogElement element{};
            element.type_ = LogType::DOUBLE;
            element.u_.d = value;
            pushValue(element);
        }

        template<typename T,typename... A>
        void log(const char *s,const T &value,A... args) noexcept{
            while(*s){
                if(*s=='%'){
                    if (UNLIKELY(*(s + 1) == '%')) {
                        ++s;
                    }
                    else{
                        pushValue(value);
                        log(s+1,args...);
                        return;
                    }
                }
                pushValue(*s++);
            }
            FATAL("extra arguments provided to log()");
        }

        void log(const char *s) noexcept{
            while(*s){
                if(*s == '%'){
                    if(UNLIKELY(*(s+1)=='%')){
                        ++s;
                    }else{
                        FATAL("missing arguments to log()");
                    }
                }
                pushValue(*s++);
            }
        }
    };

}