#include "sc_command_handler.h"
#include <cassert>
#include <eventloop/tcp_connection.h>
#include "command_messages.h"
#include "switch_client.h"
#include "sc_context.h"
#include "utils/time.h"

using namespace evt_loop;

void SCCommandHandler::Echo(const char* content)
{
    size_t sent_bytes = SendCommandMessage(ECommand::ECHO, content);

    if (sent_bytes > 0) {
        printf("Sent ECHO message, content: %s\n", content);
    }
}

void SCCommandHandler::Register(uint32_t ep_id, EEndpointRole ep_role,
        const string& access_code, bool with_token, uint8_t svc_type)
{
    CommandRegister reg_cmd;
    reg_cmd.id = ep_id > 0 ? ep_id : client_->GetContext()->endpoint_id;

    const char* role_str = EndpointRoleToTag(ep_role);
    reg_cmd.role = role_str;
    if (! access_code.empty()) {
        reg_cmd.access_code = access_code;
    }
    const string& token = client_->GetContext()->token;
    if (with_token && !token.empty()) {
        reg_cmd.token = token;
    }
    reg_cmd.svc_type = svc_type;
    client_->GetContext()->svc_type = svc_type;

    auto content = reg_cmd.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(ECommand::REG, content);

    if (sent_bytes > 0) {
        printf("Sent REG message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::GetInfo(bool is_details, uint32_t ep_id)
{
    ECommand cmd = ECommand::INFO;
    CommandInfoReq cmd_info_req;
    cmd_info_req.is_details = is_details;
    if (ep_id > 0) {
        cmd_info_req.endpoint_id = ep_id;
        cmd = ECommand::EP_INFO;
    }

    //string content(R"({"is_details": true})");
    auto content = cmd_info_req.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(cmd, content);

    if (sent_bytes > 0) {
        printf("Sent INFO/EP_INFO message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::ForwardTargets(const vector<uint32_t>& targets)
{
    //string content(R"({"targets": [1, 2]})");
    CommandForward cmd_fwd;
    cmd_fwd.targets = targets;
    auto content = cmd_fwd.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(ECommand::FWD, content);
    if (sent_bytes > 0) {
        printf("Sent FWD message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::UnforwardTargets(const vector<uint32_t>& targets)
{
    CommandUnforward cmd_unfwd;
    cmd_unfwd.targets = targets;
    auto content = cmd_unfwd.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(ECommand::UNFWD, content);
    if (sent_bytes > 0) {
        printf("Sent UNFWD message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::Subscribe(const vector<uint32_t>& sources, const vector<uint8_t>& messages)
{
    SubUnsubRejUnrej<CommandSubscribe>(ECommand::SUB, sources, messages);
}

void SCCommandHandler::Unsubscribe(const vector<uint32_t>& sources, const vector<uint8_t>& messages)
{
    SubUnsubRejUnrej<CommandUnsubscribe>(ECommand::UNSUB, sources, messages);
}

void SCCommandHandler::Reject(const vector<uint32_t>& sources, const vector<uint8_t>& messages)
{
    SubUnsubRejUnrej<CommandReject>(ECommand::REJECT, sources, messages);
}

void SCCommandHandler::Unreject(const vector<uint32_t>& sources, const vector<uint8_t>& messages)
{
    SubUnsubRejUnrej<CommandUnreject>(ECommand::UNREJECT, sources, messages);
}

template<typename T>
void SCCommandHandler::SubUnsubRejUnrej(ECommand cmd, const vector<uint32_t>& sources, const vector<uint8_t>& messages)
{
    T cmd_obj;
    cmd_obj.sources = sources;
    cmd_obj.messages = messages;
    auto content = cmd_obj.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(cmd, content);
    if (sent_bytes > 0) {
        printf("Sent %s message, content: %s\n", CommandToTag(cmd), content.c_str());
    }
}

void SCCommandHandler::Publish(const string& data, const vector<uint32_t> targets)
{
    auto cmd = ECommand::PUBLISH;
    string pub_msg_bytes;
    if (! targets.empty()) {
        cmd = ECommand::PUBLISH_2;
        PublishingMessage pub_msg;
        pub_msg.source = client_->GetContext()->endpoint_id;
        pub_msg.n_targets = targets.size();

        pub_msg_bytes.append((char*)&pub_msg, sizeof(pub_msg));
        pub_msg_bytes.append((char*)targets.data(), targets.size() * sizeof(targets[0]));
    }

    size_t sent_bytes = SendCommandMessage(cmd, data, pub_msg_bytes);
    if (sent_bytes > 0) {
        printf("Sent PUBLISH/PUBLISH_2 message, content size(%ld):\n", data.size());
        cout << DumpHexWithChars(data, evt_loop::DUMP_MAX_BYTES) << endl;
    }
}

void SCCommandHandler::RequestService(const string& data, uint8_t svc_type)
{
    ServiceMessage svc_msg;
    svc_msg.svc_type = svc_type;
    svc_msg.svc_cmd = 1;
    svc_msg.sess_id = 10;
    svc_msg.source = client_->GetContext()->endpoint_id;
    //svc_msg.svc_type = svc_type > 0 ? svc_type : client_->GetContext()->svc_type;
    string svc_msg_bytes((char*)&svc_msg, sizeof(svc_msg));
    size_t sent_bytes = SendCommandMessage(ECommand::SVC, data, svc_msg_bytes);
    printf("Sent SVC message, content size(%ld):\n", data.size());
    if (sent_bytes > 0) {
        cout << DumpHexWithChars(data, evt_loop::DUMP_MAX_BYTES) << endl;
    }
}

void SCCommandHandler::Setup(const string& access_code, const string& new_admin_code,
        const string& new_access_code, const string& mode)
{
    CommandSetup cmd_setup;
    cmd_setup.access_code = access_code;
    cmd_setup.new_admin_code = new_admin_code;
    cmd_setup.new_access_code = new_access_code;
    cmd_setup.mode = mode;

    auto content = cmd_setup.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(ECommand::SETUP, content);
    if (sent_bytes > 0) {
        printf("Sent SETUP message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::Kickout(const vector<uint32_t>& targets)
{
    //string content(R"({"targets": [95]})");
    CommandKickout cmd_kickout;
    cmd_kickout.targets = targets;
    auto content = cmd_kickout.encodeToJSON();
    size_t sent_bytes = SendCommandMessage(ECommand::KICKOUT, content);
    if (sent_bytes > 0) {
        printf("Sent KICKOUT message, content: %s\n", content.c_str());
    }
}

void SCCommandHandler::Reload()
{
    size_t sent_bytes = SendCommandMessage(ECommand::RELOAD, "");
    if (sent_bytes > 0) {
        printf("Sent RELOAD message\n");
    }
}

size_t SCCommandHandler::SendCommandMessage(ECommand cmd, const string& payload, const string& hdr_ext)
{
    return SendCommandMessage(client_->Connection().get(), cmd, payload, hdr_ext);
}

size_t SCCommandHandler::SendCommandMessage(TcpConnection* conn, ECommand cmd, const string& payload, const string& hdr_ext)
{
    if (! client_->IsConnected()) {
        printf("The connection was disconnected! Do nothing.\n");
        return 0;
    }

    CommandMessage cmdMsg;
    cmdMsg.SetCommand(cmd);
    cmdMsg.SetToJSON();
    cmdMsg.SetPayloadLen(payload.size() + hdr_ext.size());
    cmdMsg.ConvertToNetworkMessage(client_->GetMessageHeaderDescription()->is_payload_len_including_self);
    cout << "Send command message bytes size: " << sizeof(cmdMsg) << endl;
    cout << "Send command message bytes:" << endl;
    cout << DumpHex(string((char*)&cmdMsg, sizeof(cmdMsg))) << endl;

    conn->Send((char*)&cmdMsg, sizeof(cmdMsg));
    size_t sent_bytes = sizeof(cmdMsg);
    if (! hdr_ext.empty()) {
        conn->Send(hdr_ext);
        sent_bytes += hdr_ext.size();
        cout << DumpHex(hdr_ext) << endl;
    }
    if (! payload.empty()) {
        conn->Send(payload);
        sent_bytes += payload.size();
        cout << DumpHex(payload) << endl;
    }
    cout << "Send command message total bytes size: " << sent_bytes << endl;

    return sent_bytes;
}

void SCCommandHandler::HandleCommandResult(TcpConnection* conn, CommandMessage* cmdMsg)
{
    ECommand cmd = cmdMsg->Command();
    printf("Command result:\n");
    printf("cmd: %s(%d)\n", CommandToTag(cmd), uint8_t(cmd));

    auto resultMsg = cmdMsg->GetResultMessage();
    int8_t errcode = resultMsg->errcode;
    const char* content = resultMsg->data;
    printf("errcode: %d\n", errcode);

    size_t content_len = cmdMsg->GetResultMessageContentSize();
    printf("content len: %ld\n", content_len);
    if (errcode == 0) {
        printf("content: %s\n", content);
        //cout << DumpHexWithChars(content, content_len, evt_loop::DUMP_MAX_BYTES) << endl;
    } else {
        printf("error message: %s\n", content);
    }

    switch (cmd)
    {
        case ECommand::REG:
            if (errcode == 0) {
                HandleRegisterResult(cmdMsg, content);
                client_->GetContext()->register_errmsg = "";
            } else {
                client_->GetContext()->register_errmsg = content;
            }
            break;
        case ECommand::INFO:
            if (errcode == 0) {
                HandleGetInfoResult(cmdMsg, content);
            }
            break;
        case ECommand::EP_INFO:
            if (errcode == 0) {
                HandleGetEndpointInfoResult(cmdMsg, content);
            }
            break;
        case ECommand::SVC:
            if (errcode == 0) {
                HandleServiceResult(cmdMsg, content);
            }
            break;
        default:
            break;
    }
}

void SCCommandHandler::HandleRegisterResult(CommandMessage* cmdMsg, const string& payload)
{
    CommandResultRegister reg_result;
    if (cmdMsg->IsJSON()) {
        reg_result.decodeFromJSON(payload);
    } else if (cmdMsg->IsPB()) {
        reg_result.decodeFromPB(payload);
    } else {
        assert(false && "Unsupported message codec");
    }
    cout << "endpoint id: " << reg_result.id << endl;
    cout << "token: " << reg_result.token << endl;
    cout << "role: " << reg_result.role << endl;

    auto context = client_->GetContext();
    context->is_registered = true;
    context->endpoint_id = reg_result.id;
    if (! reg_result.token.empty()) {
        context->token = reg_result.token;
    }
    if (! reg_result.role.empty()) {
        context->role = TagToEndpointRole(reg_result.role);
    }
}

void SCCommandHandler::HandleGetInfoResult(CommandMessage* cmdMsg, const string& data)
{
    CommandInfo cmd_info;
    if (cmdMsg->IsJSON()) {
        cmd_info.decodeFromJSON(data);
    } else if (cmdMsg->IsPB()) {
        cmd_info.decodeFromPB(data);
    } else {
        assert(false && "Unsupported message codec");
    }
    printf("> cmd_info.uptime: %s\n", readable_seconds_delta(cmd_info.uptime).c_str());
    printf("> cmd_info.endpoints.total: %d\n", cmd_info.endpoints.total);
    printf("> cmd_info.endpoints.rx_bytes: %d\n", cmd_info.endpoints.rx_bytes);
    printf("> cmd_info.admin_endpoints.total: %d\n", cmd_info.admin_endpoints.total);
}

void SCCommandHandler::HandleGetEndpointInfoResult(CommandMessage* cmdMsg, const string& data)
{
    CommandEndpointInfo cmd_ep_info;
    if (cmdMsg->IsJSON()) {
        cmd_ep_info.decodeFromJSON(data);
    } else if (cmdMsg->IsPB()) {
        cmd_ep_info.decodeFromPB(data);
    } else {
        assert(false && "Unsupported message codec");
    }
    printf("> cmd_ep_info.uptime: %s\n", readable_seconds_delta(cmd_ep_info.uptime).c_str());
}

// handle the data that published from other endpoints
void SCCommandHandler::HandlePublishData(TcpConnection* conn, CommandMessage* cmdMsg)
{
    printf("Received forwarding PUBLISH/PUBLISH_2 message:\n");
    ECommand cmd = cmdMsg->Command();
    printf("Command message:\n");
    printf("cmd: %s(%d)\n", CommandToTag(cmd), uint8_t(cmd));
    auto [payload, payload_len] = cmdMsg->Payload();
    printf("payload_len: %d\n", payload_len);

    auto pub_msg = cmdMsg->GetPublishingMessage();
    if (pub_msg) {
        printf("source: %d\n", pub_msg->source);
        printf("n_targets: %d\n", pub_msg->n_targets);
        for (int i=0; i<pub_msg->n_targets; i++) {
            printf("%d, ", pub_msg->targets[i]);
        }
        printf("\n");
    }

    cout << DumpHexWithChars(payload, payload_len, evt_loop::DUMP_MAX_BYTES) << endl;
}

void SCCommandHandler::HandleServiceRequest(TcpConnection* conn, CommandMessage* cmdMsg)
{
    printf("Received SVC reqeust message:\n");
    ECommand cmd = cmdMsg->Command();
    printf("Command message:\n");
    printf("cmd: %s(%d)\n", CommandToTag(cmd), uint8_t(cmd));
    printf("payload_len: %d\n", cmdMsg->PayloadLen());

    auto svc_msg = cmdMsg->GetServiceMessage();
    if (svc_msg) {
        printf("svc type: %d\n", svc_msg->svc_type);
        printf("svc cmd: %d\n", svc_msg->svc_cmd);
        printf("sess_id: %d\n", svc_msg->sess_id);
        printf("source: %d\n", svc_msg->source);
    }

    ResultMessage result_msg;
    result_msg.errcode = 0;
    string rsp_payload(R"({"svc": "result"})");

    cmdMsg->SetResponseFlag();
    cmdMsg->SetPayloadLen(sizeof(ServiceMessage) + sizeof(ResultMessage) + rsp_payload.size());
    cmdMsg->ConvertToNetworkMessage(client_->GetMessageHeaderDescription()->is_payload_len_including_self);

    conn->Send(cmdMsg->Data(), cmdMsg->HeaderSize());
    conn->Send((char*)svc_msg, sizeof(ServiceMessage));
    conn->Send((char*)&result_msg, sizeof(result_msg));
    conn->Send(rsp_payload);

    // TODO: handle service request, handle by svc_msg->svc_cmd
}

void SCCommandHandler::HandleServiceResult(CommandMessage* cmdMsg, const string& content)
{
    printf("Received SVC response message:\n");
    ECommand cmd = cmdMsg->Command();
    printf("Command message:\n");
    printf("cmd: %s(%d)\n", CommandToTag(cmd), uint8_t(cmd));
    printf("payload_len: %d\n", cmdMsg->PayloadLen());

    auto svc_msg = cmdMsg->GetServiceMessage();
    if (svc_msg) {
        printf("svc type: %d\n", svc_msg->svc_type);
        printf("svc cmd: %d\n", svc_msg->svc_cmd);
        printf("sess_id: %d\n", svc_msg->sess_id);
        printf("source: %d\n", svc_msg->source);
    }
}
