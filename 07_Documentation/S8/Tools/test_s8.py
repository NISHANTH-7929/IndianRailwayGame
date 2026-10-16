"""S8 host contract tests including negative cases. Fixtures are metadata only."""
import copy
import json
import tempfile
import unittest
from pathlib import Path
from validate_s8 import DATA,DOC,load,validate_catalog,validate_record,compatible,run
from measure_s8 import summarize
from instantiate_content import instantiate

def asset():
    return dict(assetId='Loco.Test.01',name='SM_Test',category='locomotive',state='placeholder',sourceFormat='FBX',
        unrealPath='/Game/IndianRailway/Production/Test/SM_Test',importType='StaticMesh',scaleCmPerUnit=1,forward='+X',up='+Z',pivot='body centre rail reference',
        collision='simple-proxy',materials=['M_Shared'],materialSlotLimit=None,textures=[],lods=['LOD0','LOD1'],materialLodProfile='Material.Mobile.01',chunk=1,essential=True,
        platforms=['Android'],tiers=['Low'],dependencies=[],localizationKeys=[],audioDependencies=[],variants=[],external=False,provenance=None,owner='test-only')

class Foundation(unittest.TestCase):
    def setUp(self):
        self.m=load(DATA/'manifest.json');self.g=load(DATA/'graphics.json');self.l=load(DATA/'localization.json')
    def errors(self): return validate_catalog(self.m,self.g,self.l)[0]
    def saved(self): return dict(copy.deepcopy(self.m['saveCompatibility']),contentId=self.m['contentId'],contentVersion=self.m['contentVersion'])
    def test_F01_manifest_loads(self): self.assertEqual(run()['status'],'PASS')
    def test_F02_versions(self):
        for version in ['1','1.0','01.0.0','1.0.-1','x']:
            self.m['contentVersion']=version;self.assertTrue(self.errors())
        self.m['contentVersion']='0.1.0';self.m['compatiblePreviousVersions']=['0.2.0'];self.assertTrue(self.errors())
    def test_F03_stable_identity(self):
        a=asset();self.m['assets']=[a];self.m['vehicleBindings'][0]['exterior']=a['assetId'];self.assertFalse(self.errors())
        a['unrealPath']='/Game/IndianRailway/Production/Replaced/SM_Other';a['name']='SM_Other';self.assertFalse(self.errors());self.assertEqual(self.m['vehicleBindings'][0]['exterior'],a['assetId'])
    def test_F04_invalid_metadata(self):
        for key,value in [('assetId',''),('name','wrong'),('scaleCmPerUnit',0),('lods',[]),('materials',None),('chunk',999),('tiers',['Ultra'])]:
            a=asset();a[key]=value;self.m['assets']=[a];self.assertTrue(self.errors(),key)
    def test_F05_provenance(self):
        p=load(DOC/'Templates/provenance.json');self.assertFalse(validate_record('provenance',p));p['VerificationStatus']='invented';self.assertTrue(validate_record('provenance',p))
        p.update(VerificationStatus='approved',CommercialUse='verified',ModificationPermission='verified',VerificationEvidence=['test-only-review'],AcquisitionDate='2026-09-29',License='test-only-license',Creator='test-creator',Source='test-source',SourceReference='test-reference',OriginalArchive='test-archive',RedistributionRestrictions='test restrictions reviewed',AttributionRequirement='test attribution reviewed')
        self.assertFalse(validate_record('provenance',p,True));p['AcquisitionDate']='2026-02-30';self.assertTrue(validate_record('provenance',p,True))
    def test_F06_unverified_license(self):
        p=load(DOC/'Templates/provenance.json');self.assertTrue(validate_record('provenance',p,True));p['VerificationStatus']='approved';self.assertTrue(validate_record('provenance',p))
        a=asset();a['external']=True;self.m['assets']=[a];self.assertTrue(self.errors())
    def test_F07_vehicle(self):
        self.assertEqual(self.m['vehicleBindings'][0]['definitionId'],1);self.m['vehicleBindings'][0]['exterior']='Loco.Missing.01';self.assertTrue(self.errors())
    def test_F08_station(self):
        self.assertEqual(self.m['stationBindings'][0]['platforms'][0]['logicalEdge'],2);self.m['stationBindings'][0]['canopy']='Station.Missing.01';self.assertTrue(self.errors())
    def test_F09_audio(self):
        self.assertEqual(self.m['audioProfiles'][0]['layers']['traction']['semanticState'],'handling.tractionForce');self.m['audioProfiles'][0]['layers']['horn']['asset']='Audio.Missing.01';self.assertTrue(self.errors())
    def test_F10_graphics(self):
        self.assertEqual(set(self.g['tiers']),{'Low','Medium','High'});self.g['tiers']['Low']['physicsRate']=30;self.assertTrue(self.errors())
    def test_F11_localization(self):
        self.assertFalse(self.errors());self.l['strings'].pop();self.assertTrue(self.errors())
    def test_F12_streaming(self):
        self.m['chunks'][0]['level']='/Game/Wrong';self.assertTrue(self.errors())
    def test_F13_dependencies(self):
        a=asset();a['dependencies']=[a['assetId']];self.m['assets']=[a];self.assertTrue(self.errors());a['dependencies']=['Loco.Missing.01'];self.assertTrue(self.errors())
    def test_F14_save_compatibility(self):
        s=self.saved();self.assertFalse(compatible(s,self.m));self.m['contentVersion']='0.2.0';self.assertTrue(compatible(s,self.m));self.m['compatiblePreviousVersions']=['0.1.0'];self.assertFalse(compatible(s,self.m))
        for key in ['snapshotSchema','serviceVersion','vehicleDefinitions','routeContract','serviceId']:
            wrong=copy.deepcopy(s);wrong[key]=None;self.assertTrue(compatible(wrong,self.m),key)
    def test_F15_measurement(self):
        with tempfile.TemporaryDirectory() as folder:
            csv=Path(folder)/'synthetic.csv';csv.write_text('EVENTS,FrameTime,GameThreadTime,GPUTime\n,20,5,0\n,40,6,0\nEVENTS,FrameTime,GameThreadTime,GPUTime\n[HasHeaderRowAtEnd]\n',encoding='utf-8')
            p=load(DATA/'profile.json');p.update(warmupSeconds=0,endSeconds=.06)
            result=summarize(csv,p);self.assertAlmostEqual(result['metrics']['FPS']['value'],1000/30);self.assertEqual(result['metrics']['FrameTime']['p95'],40);self.assertEqual(result['metrics']['GPUTime']['status'],'UNVALIDATED');json.dumps(result,allow_nan=False)
            csv.write_text('EVENTS,FrameTime\n,20\n');self.assertRaises(ValueError,summarize,csv,p)
    def test_F16_workload(self):
        w=load(DATA/'workload.json');self.assertFalse(validate_record('workload',w));self.assertEqual(len(w['combinations']),4);self.assertTrue(w['missingProductionContent'])
    def test_F17_cost(self):
        c=load(DOC/'Templates/cost.json');self.assertFalse(validate_record('cost',c));c['finalAcceptedState']='accepted';self.assertTrue(validate_record('cost',c))
    def test_F18_repeatability(self):
        r=load(DOC/'Templates/repeatability.json');r.update(firstContent='Loco.Example.01',secondContent='Loco.Example.02');self.assertFalse(validate_record('repeatability',r));r['status']='passed';self.assertTrue(validate_record('repeatability',r))
        with tempfile.TemporaryDirectory() as folder:
            records=instantiate('Loco.Example.01','locomotive',folder,'Loco.Example.02')
            self.assertEqual(records['repeatability']['status'],'pending')
            self.assertRaises(ValueError,instantiate,'Loco.Example.01','locomotive',folder)
    def test_duplicate_ids(self):
        self.m['assets']=[asset(),asset()];self.assertTrue(self.errors())
    def test_false_readiness(self):
        r=load(DOC/'Templates/readiness.json');r['checks']['android']['status']='passed';self.assertTrue(validate_record('readiness',r))
    def test_release_empty_fails(self): self.assertTrue(validate_catalog(self.m,self.g,self.l,True)[0])
    def test_duplicate_json_keys(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'duplicate.json';p.write_text('{"id":1,"id":2}');self.assertRaises(ValueError,load,p)
    def test_nonfinite_json(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'bad.json';p.write_text('{"value":NaN}');self.assertRaises(ValueError,load,p)

class Results(unittest.TextTestResult):
    def __init__(self,*args,**kwargs): super().__init__(*args,**kwargs);self.records=[]
    def addSuccess(self,test): super().addSuccess(test);self.records.append(dict(name=test.id(),status='PASS'))
    def addFailure(self,test,err): super().addFailure(test,err);self.records.append(dict(name=test.id(),status='FAIL',detail=self._exc_info_to_string(err,test)))
    def addError(self,test,err): super().addError(test,err);self.records.append(dict(name=test.id(),status='ERROR',detail=self._exc_info_to_string(err,test)))

if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2,resultclass=Results).run(unittest.defaultTestLoader.loadTestsFromTestCase(Foundation))
    (DOC/'Evidence/foundation-tests.json').write_text(json.dumps(dict(scope='Host metadata and synthetic harness tests; not device validation',passed=result.wasSuccessful(),tests=result.records),indent=2)+'\n',encoding='utf-8')
    raise SystemExit(not result.wasSuccessful())
