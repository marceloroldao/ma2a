#include <iostream>
#include <string>
#include <vector>
#include <resolutive_routing/contracts.hpp>
#include <resolutive_routing/failure_adapter.hpp>
#include <resolutive_routing/reroute.hpp>
#include <resolutive_routing/router.hpp>
using namespace resolutive_routing;
static NodeSnapshot node(std::string id,double latency,double reputation){NodeSnapshot n;n.node_id=std::move(id);n.organization_id="org-1";n.trusted=true;n.available=true;n.compute_capacity=100.0;n.current_load=0.1;n.latency_ms=latency;n.reputation=reputation;n.supported_scopes={Scope::Private};return n;}
int main(int argc,char** argv){
 if(argc!=5&&argc!=8)return 2;
 Request request;request.request_id=argv[1];request.type=RequestType::Echo;request.scope=Scope::Private;request.source_node_id="node-a";request.organization_id="org-1";
 const std::vector<NodeSnapshot> nodes{node("node-b",5.0,1.0),node("node-c",20.0,0.90),node("node-d",35.0,0.80)};
 DeterministicRouter router;const auto first=router.route(request,nodes);if(!first.selected_node_id||*first.selected_node_id!="node-b")return 3;
 AuthenticatedFailureNoticeView n1;n1.request_id=argv[1];n1.failed_node_id=argv[2];n1.reason=argv[3];n1.observed_at=std::stoll(argv[4]);n1.authenticated=true;const auto f1=failure_event_from_authenticated_notice(n1);
 const auto second=reroute_after_failure(router,request,nodes,first,f1);if(!second.selected_node_id||*second.selected_node_id!="node-c")return 4;
 std::cout<<*first.selected_node_id<<"\n"<<*second.selected_node_id<<"\n";if(argc==5)return 0;
 AuthenticatedFailureNoticeView n2;n2.request_id=argv[1];n2.failed_node_id=argv[5];n2.reason=argv[6];n2.observed_at=std::stoll(argv[7]);n2.authenticated=true;const auto f2=failure_event_from_authenticated_notice(n2);
 const auto third=reroute_after_failure(router,request,nodes,second.route,f2,second.excluded_nodes);if(!third.selected_node_id||*third.selected_node_id!="node-d")return 5;
 std::cout<<*third.selected_node_id<<"\n";return 0;
}
