#ifndef _SC_CONSOLE
#define _SC_CONSOLE

#include "switch_message.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

using std::string;
using std::vector;

class SwitchClient;
class SCCommandHandler;

class CommandResultRegister;
class CommandInfo;
class CommandEndpointInfo;

class SCConsole {
    public:
    SCConsole(SwitchClient* client, SCCommandHandler* cmd_handler, const char* sub_prompt=nullptr);
    void Destory();

    void registerCommands();

    private:
    int handleConsoleCommand_Exit(const vector<string>& argv);
    int handleConsoleCommand_Reconnect(const vector<string>& argv);
    int handleConsoleCommand_Heartbeat(const vector<string>& argv);
    int handleConsoleCommand_Options(const vector<string>& argv);
    int handleConsoleCommand_Status(const vector<string>& argv);
    int handleConsoleCommand_Echo(const vector<string>& argv);
    int handleConsoleCommand_Register(const vector<string>& argv);
    int handleConsoleCommand_GetInfo(const vector<string>& argv);
    int handleConsoleCommand_Publish(const vector<string>& argv);
    int handleConsoleCommand_RequestService(const vector<string>& argv);
    int handleConsoleCommand_ForwardTargets(const vector<string>& argv);
    int handleConsoleCommand_UnforwardTargets(const vector<string>& argv);
    int handleConsoleCommand_Subscribe(const vector<string>& argv);
    int handleConsoleCommand_Unsubscribe(const vector<string>& argv);
    int handleConsoleCommand_Reject(const vector<string>& argv);
    int handleConsoleCommand_Unreject(const vector<string>& argv);
    int handleConsoleCommand_Setup(const vector<string>& argv);
    int handleConsoleCommand_Kickout(const vector<string>& argv);
    int handleConsoleCommand_Reload(const vector<string>& argv);

    using SetTargetsCommandCallback = std::function<void (const vector<EndpointId>&)>;
    int handleConsoleCommand_SetTargets(const vector<string>& argv, const char* desc,
            const SetTargetsCommandCallback& cmd_handler_callback);

    using SubUnsubRejUnrejCommandCallback = std::function<void (const vector<EndpointId>&, const vector<MessageId>&)>;
    int handleConsoleCommand_SubUnsubRejUnrej(const vector<string>& argv, const char* desc,
            const SubUnsubRejUnrejCommandCallback& cmd_handler_callback);


    void onCommandSuccess(ECommand cmd, const char* content, size_t content_len);
    void onCommandFail(ECommand cmd, const char* content, size_t content_len);

    void onRegisterResult(const CommandResultRegister* reg_result);
    void onGetInfoResult(const CommandInfo* cmd_info);
    void onGetEndpointInfoResult(const CommandEndpointInfo* cmd_ep_info);
    void onPublishingResult(const ResultMessage* result_msg, const char* data, size_t data_len);
    void onRequestServiceResult(const ServiceMessage* svc_msg, const char* data, size_t data_len);

    private:
    SwitchClient* client_;
    SCCommandHandler* cmd_handler_;
};
using SCConsolePtr = std::shared_ptr<SCConsole>;

#endif  // _SC_CONSOLE
