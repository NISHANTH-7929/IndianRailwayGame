"""Author the S8 data-only foundation. No art generation or asset import."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / '03_UnrealProject/MyProject/Content/IndianRailway/Production'
DOC = ROOT / '07_Documentation/S8'

def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

def obj(properties):
    return dict(type='object', properties=properties, required=list(properties), additionalProperties=False)

def arr(items, **kw):
    return dict(type='array', items=items, **kw)

def enum(*values):
    return dict(enum=list(values))

S = dict(type='string', minLength=1)
N = dict(type='number', minimum=0)
I = dict(type='integer', minimum=1)
B = dict(type='boolean')
ID = dict(type='string', pattern=r'^[A-Z][A-Za-z0-9]*(\.[A-Za-z0-9]+){2,}$')
VER = dict(type='string', pattern=r'^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$')
REF = dict(anyOf=[ID, dict(type='null')])
CATEGORIES = ['locomotive','coach','wagon','formation','station','trackside','environment','building','vegetation','character','animation','audio','vfx','ui']
TIERS = ['Low','Medium','High']
LAYERS = {'traction':'handling.tractionForce', 'rolling':'handling.speed', 'brakes':'handling.appliedBrake',
          'horn':'command.horn', 'doors':'service.stationPhase', 'compressorFans':'presentation.auxiliaryState',
          'cabControls':'command.accepted', 'stationAmbience':'presentation.stationResidency',
          'announcements':'service.stationPhase', 'environmentAmbience':'presentation.region', 'rain':'presentation.weather'}
CHECKS = ['stableAssetId','sourceRecorded','licenseVerified','commercialUseVerified','modificationRightsVerified',
          'scale','orientation','pivot','materials','textures','lods','collisionProxy','animationBinding',
          'audioBinding','streaming','platformTier','android','regression','finalApproval']

schemas = {}
schemas['provenance'] = obj(dict(AssetId=ID, AssetName=S, Category=enum(*CATEGORIES), Creator=S, Source=S,
    SourceReference=S, AcquisitionDate=dict(type=['string','null'], pattern=r'^\d{4}-\d{2}-\d{2}$'), License=S,
    CommercialUse=enum('verified','denied','unknown'), ModificationPermission=enum('verified','denied','unknown'),
    RedistributionRestrictions=S, AttributionRequirement=S, OriginalArchive=S, ModifiedAssetReference=S,
    VerificationStatus=enum('pending review','approved','rejected','restricted','unknown'),
    VerificationEvidence=arr(S), Notes=S))
schemas['asset'] = obj(dict(assetId=ID, name=dict(type='string',pattern=r'^(SM|SK|DA|ABP|AS|SW|NS|T|M|WBP)_[A-Za-z0-9_]+$'),
    category=enum(*CATEGORIES), state=enum('placeholder','candidate','accepted'), sourceFormat=S,
    unrealPath=dict(type='string',pattern=r'^/Game/IndianRailway/'), importType=S,
    scaleCmPerUnit=dict(type='number',exclusiveMinimum=0), forward=enum('+X'), up=enum('+Z'), pivot=S,
    collision=enum('none','query-proxy','simple-proxy'), materials=arr(S), materialSlotLimit=dict(type=['integer','null'],minimum=1),
    textures=arr(obj(dict(name=dict(type='string',pattern=r'^T_[A-Za-z0-9_]+_(BC|N|ORM|E|A)$'),group=S,maxDimension=I))),
    lods=arr(S,minItems=1,uniqueItems=True), materialLodProfile=ID, chunk=I, essential=B,
    platforms=arr(enum('Android','Windows'),minItems=1,uniqueItems=True), tiers=arr(enum(*TIERS),minItems=1,uniqueItems=True),
    dependencies=arr(ID,uniqueItems=True), localizationKeys=arr(S,uniqueItems=True), audioDependencies=arr(ID,uniqueItems=True),
    variants=arr(obj(dict(platform=enum('Android','Windows'),tier=enum(*TIERS),unrealPath=S))),
    external=B, provenance=dict(anyOf=[dict(type='null'),schemas['provenance']]), owner=S))
vehicle_slots=['exterior','cab','bogie','wheel','lights','animation','audio','materialLod']
station_slots=['platformVisualSet','signage','canopy','footbridge','furniture','lighting','peopleActivity','ambientAudio','regionalKit','landmarks']
schemas['manifest'] = obj(dict(schemaVersion=enum(1),contentId=ID,contentVersion=VER,compatiblePreviousVersions=arr(VER,uniqueItems=True),
    simulationContentId=enum('IR-S6-fixture-1'),scope=enum('foundation'),
    chunks=arr(obj(dict(id=I,level=S,rebindKey=S,bundle=enum('route','station','vehicle','audio','optional-scenery'),essential=B)),minItems=1),
    assets=arr(schemas['asset']),
    vehicleBindings=arr(obj(dict(definitionId=I,definitionVersion=I,presentationId=ID,**{k:REF for k in vehicle_slots}))),
    formationBindings=arr(obj(dict(formationId=ID,vehiclePresentationIds=arr(ID,minItems=1)))),
    stationBindings=arr(obj(dict(stationId=ID,serviceId=S,platforms=arr(obj(dict(platformId=ID,logicalEdge=I,fromMetres=N,toMetres=N)),minItems=1),chunk=I,**{k:REF for k in station_slots}))),
    routeBindings=arr(obj(dict(routeId=I,chunks=arr(I,minItems=1),stationIds=arr(ID)))),
    audioProfiles=arr(obj(dict(profileId=ID,layers=obj({k:obj(dict(asset=REF,semanticState=enum(v))) for k,v in LAYERS.items()})))),
    materialLodProfiles=arr(obj(dict(profileId=ID,status=enum('PROVISIONAL','MEASURE','ACCEPT','FREEZE'),materialSlotLimit=dict(type=['integer','null'],minimum=1),textureDimensionLimit=dict(type=['integer','null'],minimum=1),lodPolicy=S))),
    saveCompatibility=obj(dict(snapshotSchema=enum(1),simulationContentId=enum('IR-S6-fixture-1'),serviceId=S,serviceVersion=S,
        serviceConfigurationPolicy=enum('exact-S7-header'),vehicleDefinitions=arr(obj(dict(id=I,version=I))),routeContract=S))))
schemas['graphics'] = obj(dict(schemaVersion=enum(1),status=enum('PROVISIONAL'),defaultTier=enum(*TIERS),
    tiers=obj({tier:obj({key:dict(type=['number','null'],minimum=0) for key in ['textureResolution','lodDistance','vegetationDensity','peopleDensity','shadowQuality','vfxDensity','optionalDetail']}) for tier in TIERS})))
schemas['localization'] = obj(dict(schemaVersion=enum(1),defaultCulture=enum('en'),cultures=enum(['en','hi']),
    strings=arr(obj(dict(key=S,en=S,hi=S)),minItems=1),accessibility=obj(dict(fontScaleOptions=arr(dict(type='number',minimum=1),minItems=1),
    signalMeaning=enum('color+text+symbol'),touchLayouts=arr(enum('standard','large'),minItems=2),controlLabelKeys=arr(S,minItems=1),
    pauseKey=S,resultKey=S,warningKey=S,fontCoverageStatus=enum('pending-device-verification')))))
schemas['workload'] = obj(dict(schemaVersion=enum(1),workloadId=ID,status=enum('defined-not-executed'),
    requirements=arr(S,minItems=10,uniqueItems=True),combinations=arr(obj(dict(id=S,time=enum('day','night'),weather=enum('dry','rain'),demand=enum('normal','demanding'))),minItems=4),
    missingProductionContent=enum(True),executionEvidence=arr(S)))
step = obj(dict(description=S,effortMinutes=dict(type=['number','null'],minimum=0),evidence=arr(S)))
schemas['cost'] = obj(dict(assetId=ID,category=enum(*CATEGORIES),source=S,toolChain=arr(S),
    **{k:arr(step) for k in ['preparation','manual','correction','integration','androidOptimization','testIterations','failuresRework','aiAssistance','verification']},
    finalAcceptedState=enum('not-started','pending','accepted','rejected'),acceptanceEvidence=arr(S)))
schemas['readiness'] = obj(dict(assetId=ID,checks=obj({k:obj(dict(status=enum('pending','passed','not-applicable','failed'),evidence=arr(S),reason=dict(type='string'))) for k in CHECKS})))
schemas['repeatability'] = obj(dict(firstContent=ID,secondContent=ID,status=enum('pending','passed','failed'),
    contentEvidence=arr(S),sourceChangeManifest=S,checks=obj({k:obj(dict(status=enum('pending','passed','failed'),evidence=arr(S))) for k in ['samePipeline','noPhysicsRewrite','noAuthorityRewrite','noDuplicateAI','noDuplicateScoring','noLevelBlueprintLogic','noSpecialWidgets']})))
schemas['profile'] = obj(dict(schemaVersion=enum(1),profileId=ID,device=enum('iQOO Neo 9 Pro'),androidApi=enum(36),abi=enum('arm64-v8a'),renderer=enum('Vulkan'),textureFormat=enum('ASTC'),
    captureFrames=I,warmupSeconds=N,endSeconds=N,frameCap=enum(30),repetitions=I,metrics=arr(S,minItems=10),
    workloadId=ID,scope=enum('foundation-S7-fixture'),finalBudgetClaim=enum(False)))
for name,schema in schemas.items():
    schema['$schema']='https://json-schema.org/draft/2020-12/schema'
    schema['$id']=f'urn:indianrailway:s8:{name}'
    write(DATA/'Schemas'/f'{name}.schema.json',schema)

manifest=dict(schemaVersion=1,contentId='Content.Foundation.01',contentVersion='0.1.0',compatiblePreviousVersions=[],
 simulationContentId='IR-S6-fixture-1',scope='foundation',
 chunks=[dict(id=i,level=f'/Game/IndianRailway/S6/L_Chunk{letter}',rebindKey=key,bundle='route',essential=True) for i,letter,key in [(1,'A','west'),(2,'B','east')]],assets=[],
 vehicleBindings=[dict(definitionId=i,definitionVersion=1,presentationId=f'Vehicle.Proxy.0{i}',**{k:('Audio.Foundation.01' if k=='audio' else 'Material.Mobile.01' if k=='materialLod' else None) for k in vehicle_slots}) for i in [1,2]],
 formationBindings=[dict(formationId='Formation.Foundation.01',vehiclePresentationIds=['Vehicle.Proxy.01','Vehicle.Proxy.02'])],
 stationBindings=[dict(stationId='Station.Shantipur.01',serviceId='S7-Shantipur-01',platforms=[dict(platformId='Platform.Shantipur.01',logicalEdge=2,fromMetres=95,toMetres=145)],chunk=2,**{k:('Audio.Foundation.01' if k=='ambientAudio' else None) for k in station_slots})],
 routeBindings=[dict(routeId=2,chunks=[1,2],stationIds=['Station.Shantipur.01'])],
 audioProfiles=[dict(profileId='Audio.Foundation.01',layers={k:dict(asset=None,semanticState=v) for k,v in LAYERS.items()})],
 materialLodProfiles=[dict(profileId='Material.Mobile.01',status='PROVISIONAL',materialSlotLimit=None,textureDimensionLimit=None,lodPolicy='LOD0 plus measured reductions; non-mesh exemption requires rationale')],
 saveCompatibility=dict(snapshotSchema=1,simulationContentId='IR-S6-fixture-1',serviceId='S7-Shantipur-01',serviceVersion='IR-S7-service-1',serviceConfigurationPolicy='exact-S7-header',vehicleDefinitions=[dict(id=i,version=1) for i in [1,2]],routeContract='IR-S6-fixture-1'))
write(DATA/'manifest.json',manifest)
write(DATA/'graphics.json',dict(schemaVersion=1,status='PROVISIONAL',defaultTier='Low',tiers={t:{k:None for k in schemas['graphics']['properties']['tiers']['properties'][t]['properties']} for t in TIERS}))
write(DATA/'localization.json',dict(schemaVersion=1,defaultCulture='en',cultures=['en','hi'],strings=[dict(key=k,en=en,hi=hi) for k,en,hi in [('Control.Brake','Brake','ब्रेक'),('Menu.Pause','Pause','विराम'),('Screen.Result','Result','परिणाम'),('Warning.Stop','Stop — no movement authority','रुकें — आगे बढ़ने की अनुमति नहीं')]],accessibility=dict(fontScaleOptions=[1,1.25,1.5],signalMeaning='color+text+symbol',touchLayouts=['standard','large'],controlLabelKeys=['Control.Brake'],pauseKey='Menu.Pause',resultKey='Screen.Result',warningKey='Warning.Stop',fontCoverageStatus='pending-device-verification')))
write(DATA/'workload.json',dict(schemaVersion=1,workloadId='Workload.Representative.01',status='defined-not-executed',requirements=['long-player-formation','second-distinct-locomotive','AI-traffic','busy-station','quiet-adjoining-route','streaming-boundary','camera-changes','day','night','dry','rain','demanding-combination'],combinations=[dict(id=f'{time}-{weather}',time=time,weather=weather,demand='demanding' if time=='night' and weather=='rain' else 'normal') for time in ['day','night'] for weather in ['dry','rain']],missingProductionContent=True,executionEvidence=[]))
write(DATA/'profile.json',dict(schemaVersion=1,profileId='Profile.Foundation.01',device='iQOO Neo 9 Pro',androidApi=36,abi='arm64-v8a',renderer='Vulkan',textureFormat='ASTC',captureFrames=6000,warmupSeconds=10,endSeconds=170,frameCap=30,repetitions=3,metrics=['FPS','FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RailS6/SimulationStepMs','RailS7/ServiceUpdateMs','PSS','loadLatency','residency','activationCounts','thermalStatus','packageBytes'],workloadId='Workload.Representative.01',scope='foundation-S7-fixture',finalBudgetClaim=False))
write(DOC/'Templates/cost.json',dict(assetId='Loco.Pending.01',category='locomotive',source='pending',toolChain=[],**{k:[] for k in ['preparation','manual','correction','integration','androidOptimization','testIterations','failuresRework','aiAssistance','verification']},finalAcceptedState='not-started',acceptanceEvidence=[]))
write(DOC/'Templates/readiness.json',dict(assetId='Loco.Pending.01',checks={k:dict(status='pending',evidence=[],reason='') for k in CHECKS}))
write(DOC/'Templates/repeatability.json',dict(firstContent='Loco.Pending.01',secondContent='Loco.Pending.02',status='pending',contentEvidence=[],sourceChangeManifest='pending',checks={k:dict(status='pending',evidence=[]) for k in schemas['repeatability']['properties']['checks']['properties']}))
write(DOC/'Templates/provenance.json',dict(AssetId='Loco.Pending.01',AssetName='Pending',Category='locomotive',Creator='unknown',Source='unknown',SourceReference='unknown',AcquisitionDate=None,License='unknown',CommercialUse='unknown',ModificationPermission='unknown',RedistributionRestrictions='unknown',AttributionRequirement='unknown',OriginalArchive='unknown',ModifiedAssetReference='unknown',VerificationStatus='pending review',VerificationEvidence=[],Notes='Template only; no asset acquired.'))
write(DOC/'decision-register.json',[dict(decision=d,status='pending',evidenceNeeded=e,owner='project owner',chosenOption=None,alternatives=a,reason='Requires representative content measurements or owner selection',date='2026-09-29',affectedSystems=s) for d,e,a,s in [
 ('representative stock','two content integrations',['electric','diesel'],['vehicles']),('representative region','reference and route selection',['region A','region B'],['stations','route']),('rendering choice','Android visual and timing captures',['retain mobile forward','evaluate alternative'],['rendering']),('Android device matrix','device availability and profiles',['current iQOO','additional tiers'],['QA']),('LOD budgets','per-category reductions and visual review',['shared profiles','category profiles'],['LOD']),('material budgets','draw-call and GPU captures',['shared instances','bespoke materials'],['materials']),('texture budgets','PSS and quality captures',['ASTC sizes per tier','shared atlases'],['textures']),('streaming thresholds','load/unload and residency trials',['retain S6 demand','measured margins'],['streaming']),('people/activity budgets','busy station profiles',['instanced proxies','animated people'],['presentation']),('audio residency','voice/residency profiles',['stream','resident'],['audio']),('graphics tiers','three-tier comparison',['shared defaults','measured overrides'],['graphics']),('localization scope','Hindi glyph and user review',['core UI','broader localization'],['UI'])]])
print('Wrote data-only contracts, schemas and pending templates.')
