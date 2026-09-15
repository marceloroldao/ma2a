from __future__ import annotations
import base64,json,socket,struct,subprocess,sys,threading
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey,Ed25519PublicKey
MAX_FRAME=1024*1024
def canonical(v):return json.dumps(v,sort_keys=True,separators=(",",":")).encode()
def b64(b):return base64.urlsafe_b64encode(b).decode()
def unb64(s):return base64.b64decode(s.encode(),altchars=b"-_",validate=True)
def pub(k):return k.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
def sign(p,k):o=dict(p);o["signature"]=b64(k.sign(canonical(p)));return o
def verify(p,k):u=dict(p);s=unb64(str(u.pop("signature")));Ed25519PublicKey.from_public_bytes(k).verify(s,canonical(u))
def exact(s,n):
 b=bytearray()
 while len(b)<n:
  c=s.recv(n-len(b))
  if not c:raise RuntimeError("closed")
  b.extend(c)
 return bytes(b)
def recv(s):
 n=struct.unpack("!I",exact(s,4))[0]
 if n<=0 or n>MAX_FRAME:raise RuntimeError("frame")
 return json.loads(exact(s,n))
def send(s,m):b=canonical(m);s.sendall(struct.pack("!I",len(b))+b)
def once(h):
 l=socket.socket();l.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1);l.bind(("127.0.0.1",0));l.listen(1);port=l.getsockname()[1]
 def run():
  try:
   c,_=l.accept()
   with c:h(c)
  finally:l.close()
 t=threading.Thread(target=run,daemon=True);t.start();return port,t
def main():
 if len(sys.argv)!=2:raise SystemExit("usage: distributed_failover_with_routing.py <routing-driver>")
 driver=sys.argv[1];ka=Ed25519PrivateKey.from_private_bytes(bytes(range(32)));kb=Ed25519PrivateKey.from_private_bytes(bytes(range(32,64)));kc=Ed25519PrivateKey.from_private_bytes(bytes(range(64,96)));pa,pb,pc=pub(ka),pub(kb),pub(kc);rid="job-real-routing-1";base={"expires_at":1800000200,"issued_at":1800000000,"operation":"ECHO","organization_id":"org-1","payload":"real-routing-payload","protocol_version":"0.2","request_id":rid,"sender_node_id":"node-a","signature_algorithm":"Ed25519"}
 def fail(c):
  r=dict(recv(c)["payload"]);verify(r,pa);f={"failed_node_id":"node-b","message_id":"failure-b","observed_at":1800000100,"protocol_version":"0.2","reason":"execution_unavailable","reporting_node_id":"node-b","request_id":rid,"signature_algorithm":"Ed25519"};send(c,{"type":"FAILURE_NOTICE","payload":sign(f,kb)})
 port,t=once(fail);r=dict(base);r["message_id"]="request-b";r["target_node_id"]="node-b"
 with socket.create_connection(("127.0.0.1",port),timeout=5) as c:send(c,{"type":"JOB_REQUEST","payload":sign(r,ka)});f=dict(recv(c)["payload"])
 t.join(5);verify(f,pb);route=subprocess.run([driver,rid,str(f["failed_node_id"]),str(f["reason"]),str(f["observed_at"])],check=True,capture_output=True,text=True).stdout.strip().splitlines();assert route==["node-b","node-c"],route;target=route[1]
 def success(c):
  q=dict(recv(c)["payload"]);verify(q,pa);result={"completed_at":1800000101,"message_id":"result-c","payload":q["payload"],"protocol_version":"0.2","recipient_node_id":"node-a","request_id":rid,"responder_node_id":target,"signature_algorithm":"Ed25519","status":"OK"};send(c,{"type":"JOB_RESULT","payload":sign(result,kc)})
 port,t=once(success);r=dict(base);r["message_id"]="request-c";r["target_node_id"]=target
 with socket.create_connection(("127.0.0.1",port),timeout=5) as c:send(c,{"type":"JOB_REQUEST","payload":sign(r,ka)});result=dict(recv(c)["payload"])
 t.join(5);verify(result,pc);assert result["status"]=="OK" and result["responder_node_id"]=="node-c";print("single real routing failover OK");return 0
if __name__=="__main__":raise SystemExit(main())
