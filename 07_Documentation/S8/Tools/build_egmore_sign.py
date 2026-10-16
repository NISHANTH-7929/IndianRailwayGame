"""Separate original signage module; metric estimates, no changes to heritage kit."""
from pathlib import Path
helper=Path(__file__).with_name('build_station_kit.py')
setup=helper.read_text().split('box((2.5,2.5,.37)')[0]
# Restore the planar lettering UVs after the shared structural cube projection.
setup=setup.replace("    o.data.calc_loop_triangles()", """    for poly in o.data.polygons:
        if o.data.materials[poly.material_index].name=='EgmoreName':
            for loop_index in poly.loop_indices:
                v=o.data.vertices[o.data.loops[loop_index].vertex_index].co
                u=(v.x+2)/4 if poly.normal.y<0 else (2-v.x)/4
                o.data.uv_layers.active.data[loop_index].uv=(u,(v.z-1.45)/2)
    o.data.calc_loop_triangles()""")
setup=setup.replace("    reflection=Matrix.Diagonal", """    for poly in o.data.polygons:
        if o.data.materials[poly.material_index].name=='EgmoreName':
            for loop_index in poly.loop_indices:
                uv=o.data.uv_layers.active.data[loop_index].uv
                uv.x=1-uv.x
    reflection=Matrix.Diagonal""")
exec(compile(setup.replace('S8/StationKit','S8/EgmoreSignage'),str(helper),'exec'))
mat=bpy.data.materials.new('EgmoreName');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.75
files={s:'Textures/T_EgmoreName_'+s+'.png' for s in ['BC','N','ORM']}
for suffix,file in files.items():
    node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(OUT/file))
    if suffix=='BC':mat.node_tree.links.new(node.outputs['Color'],bs.inputs['Base Color'])
    else:node.image.colorspace_settings.name='Non-Color'
materials['EgmoreName']=mat
definitions.append({'name':'EgmoreName','base_color':[.8,.6,.1,1],'roughness':.75,'metallic':0,'textures':files})
for x in [-2.12,2.12]:
    box((x,0,1.8),(.17,.20,3.6),'Yellow')
    box((x,0,.10),(.42,.46,.20),'Concrete')
box((0,0,2.45),(4.08,.13,2.08),'Yellow')
for z in [1.42,3.48]:box((0,0,z),(4.25,.19,.07),'Yellow')
for side in [-1,1]:
    verts=[(-2,side*.071,1.45),(2,side*.071,1.45),(2,side*.071,3.45),(-2,side*.071,3.45)]
    mesh=bpy.data.meshes.new('lettering face');mesh.from_pydata(verts,[],[(0,1,2,3) if side<0 else (3,2,1,0)]);mesh.update()
    obj=bpy.data.objects.new('lettering face',mesh);scene.collection.objects.link(obj);finish_object(obj,'EgmoreName')
export('EgmoreNameboard4m')
assembly={'asset_id':'Station.Egmore.Signage.01','version':1,'units':'metres','materials':definitions,'parts':parts,
 'source':'Original structure and shaped lettering; lettering-receipt.json records reference and font',
 'status':'estimated historical nameboard study; not survey certified'}
(OUT/'assembly.json').write_text(json.dumps(assembly,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'EgmoreSignage.blend'))
obj=bpy.data.objects['SM_Station_EgmoreNameboard4m'];obj.hide_set(False);obj.hide_render=False
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
mod=obj.modifiers.new('portable triangulation','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name)
bpy.ops.export_scene.gltf(filepath=str(OUT/'EgmoreNameboard4m.glb'),export_format='GLB',use_selection=True,export_tangents=True)
print('EGMORE_SIGN_COMPLETE',parts[0]['triangles'])
