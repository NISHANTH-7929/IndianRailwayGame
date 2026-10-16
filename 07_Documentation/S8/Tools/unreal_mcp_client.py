"""Local Unreal MCP transport for this project's native editor tools.

Useful when the calling chat hasn't reloaded the plugin tool catalogue. It
discovers native schemas; no fabricated replacement editor tools are supplied.
"""
import argparse,json,urllib.request

class Client:
    def __init__(self):
        self.session=None; self.sequence=0
        self.request('initialize',dict(protocolVersion='2024-11-05',capabilities={},clientInfo=dict(name='IndianRailwayCodex',version='1')))
    def request(self,method,params):
        self.sequence+=1
        headers={'Content-Type':'application/json','Accept':'application/json, text/event-stream'}
        if self.session: headers['Mcp-Session-Id']=self.session
        req=urllib.request.Request('http://127.0.0.1:8000/mcp',data=json.dumps(dict(jsonrpc='2.0',id=self.sequence,method=method,params=params)).encode(),headers=headers)
        with urllib.request.urlopen(req,timeout=60) as r:
            self.session=r.headers.get('Mcp-Session-Id',self.session)
            raw=r.read().decode()
        if raw.startswith('event:') or raw.startswith('data:'):
            raw=next(line[5:].strip() for line in raw.splitlines() if line.startswith('data:'))
        result=json.loads(raw)
        if 'error' in result:raise RuntimeError(json.dumps(result['error']))
        return result['result']
    def tool(self,name,args):return self.request('tools/call',dict(name=name,arguments=args))
    def describe(self,toolset):
        result=self.tool('describe_toolset',dict(toolset_name=toolset))
        return json.loads(result['content'][0]['text'])

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['list','describe','call']);p.add_argument('toolset',nargs='?');p.add_argument('name',nargs='?');p.add_argument('--arguments',default='{}');p.add_argument('--filter',default='');a=p.parse_args();c=Client()
    if a.action=='list':result=c.tool('list_toolsets',{})
    elif a.action=='describe':
        result=c.describe(a.toolset)
        if a.filter:result['tools']=[t for t in result['tools'] if a.filter.lower() in t['name'].lower()]
    else:
        schema=c.describe(a.toolset)
        assert any(t['name'].split('.')[-1]==a.name for t in schema['tools']), 'Unknown native tool'
        result=c.tool('call_tool',dict(toolset_name=a.toolset,tool_name=a.name,arguments=json.loads(a.arguments)))
    print(json.dumps(result,indent=2))
