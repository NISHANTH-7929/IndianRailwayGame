from pathlib import Path
p=Path(__file__).with_name('refinement.py');s=p.read_text();s=s.replace("for a,b in zip(pts,pts[1:]):rod('Primary coil',a,b,.022,steel)","""verts=[];faces=[]
    for i,pt in enumerate(pts):
     for j in range(6):
      a=j*math.tau/6;verts.append((pt[0]+.022*math.cos(a),pt[1]+.022*math.sin(a),pt[2]+.022*math.sin(a)))
    for i in range(len(pts)-1):
     for j in range(6):faces.append((i*6+j,i*6+(j+1)%6,(i+1)*6+(j+1)%6,(i+1)*6+j))
    me=bpy.data.meshes.new('Primary coil');me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new('Primary coil',me);bpy.context.collection.objects.link(o);o.data.materials.append(steel)""");p.write_text(s)
