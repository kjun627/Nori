import math
import os
import shutil
from xml.dom.minidom import Document

import bpy
import bpy_extras
from bpy.props import BoolProperty, IntProperty, StringProperty
from bpy_extras.io_utils import ExportHelper
from mathutils import Matrix

bl_info = {
    "name": "Export Nori scenes format",
    "author": "Adrien Gruson, Delio Vicini, Tizian Zeltner (Updated for 4.x)",
    "version": (0, 3),
    "blender": (4, 0, 0),
    "location": "File > Export > Nori exporter (.xml)",
    "description": "Export Nori scene format (.xml)",
    "category": "Import-Export"}

class NoriWriter:
    def __init__(self, context, filepath):
        self.context = context
        self.filepath = filepath
        self.working_dir = os.path.dirname(self.filepath)
        self.doc = None
        self.scene = None

    def create_xml_element(self, name, attr):
        el = self.doc.createElement(name)
        for k, v in attr.items():
            el.setAttribute(k, v)
        return el

    def create_xml_entry(self, t, name, value):
        return self.create_xml_element(t, {"name": name, "value": value})

    def create_xml_transform(self, mat):
        transform = self.create_xml_element("transform", {"name": "toWorld"})
        value = ""
        for j in range(4):
            for i in range(4):
                value += f"{mat[j][i]},"
        transform.appendChild(self.create_xml_element("matrix", {"value": value[:-1]}))
        return transform

    def create_xml_mesh_entry(self, filename):
        meshElement = self.create_xml_element("shape", {"type": "obj"})
        meshElement.appendChild(self.create_xml_element("string", {"name": "filename", "value": "meshes/"+filename}))
        return meshElement

    def write(self):
        n_samples = 32
        self.doc = Document()
        self.scene = self.doc.createElement("scene")
        self.doc.appendChild(self.scene)

        # 1) Integrator
        self.scene.appendChild(self.create_xml_element("integrator", {"type": "normals"}))

        # 2) Sampler
        sampler = self.create_xml_element("sampler", {"type": "independent"})
        sampler.appendChild(self.create_xml_element("integer", {"name": "sampleCount", "value": str(n_samples)}))
        self.scene.appendChild(sampler)

        # 3) Camera
        cameras = [cam for cam in self.context.scene.objects if cam.type == 'CAMERA']
        if not cameras:
            print("WARN: No camera to export")
        else:
            self.scene.appendChild(self.write_camera(self.context.scene.camera))

        # 4) Meshes
        mesh_dir = os.path.join(self.working_dir, "meshes")
        if not os.path.exists(mesh_dir):
            os.makedirs(mesh_dir)

        meshes = [obj for obj in self.context.scene.objects if obj.type in {'MESH', 'FONT', 'SURFACE', 'META'}]
        
        for mesh in meshes:
            self.write_mesh(mesh)

        # 5) Save XML
        with open(self.filepath, "w", encoding="utf-8") as f:
            self.doc.writexml(f, "", "\t", "\n")

    def write_camera(self, cam):
        camera = self.create_xml_element("camera", {"type": "perspective"})
        camera.appendChild(self.create_xml_entry("float", "fov", str(math.degrees(cam.data.angle))))
        camera.appendChild(self.create_xml_entry("float", "nearClip", str(cam.data.clip_start)))
        camera.appendChild(self.create_xml_entry("float", "farClip", str(cam.data.clip_end)))
        
        render = self.context.scene.render
        percent = render.resolution_percentage / 100.0
        camera.appendChild(self.create_xml_entry("integer", "width", str(int(render.resolution_x * percent))))
        camera.appendChild(self.create_xml_entry("integer", "height", str(int(render.resolution_y * percent))))

        # Camera Transform (Blender to Nori)
        mat = cam.matrix_world
        # Axis conversion: Forward is -Z, Up is Y
        flip_yz = bpy_extras.io_utils.axis_conversion(from_forward='Y', from_up='Z', to_forward='-Z', to_up='Y').to_4x4()
        mat = flip_yz @ mat
        
        # Nori perspective camera fix
        m = Matrix.Scale(-1, 4, (1, 0, 0)) # Flip X
        mat = mat @ m
        
        camera.appendChild(self.create_xml_transform(mat))
        return camera

    def write_mesh(self, mesh):
        # Deselect all
        bpy.ops.object.select_all(action='DESELECT')
        mesh.select_set(True)
        self.context.view_layer.objects.active = mesh
        
        obj_name = f"{mesh.name}.obj"
        obj_path = os.path.join(self.working_dir, 'meshes', obj_name)
        
        # --- 핵심 수정 부분: Blender 4.x용 OBJ 내보내기 ---
        bpy.ops.wm.obj_export(
            filepath=obj_path,
            export_selected_objects=True,
            apply_modifiers=True,
            export_triangulated_mesh=True,
            export_normals=True,
            export_uv=True,
            forward_axis='NEGATIVE_Z',
            up_axis='Y'
        )
        
        mesh_element = self.create_xml_mesh_entry(obj_name)
        bsdf_element = self.create_xml_element("bsdf", {"type": "diffuse"})
        bsdf_element.appendChild(self.create_xml_entry("color", "albedo", "0.75,0.75,0.75"))
        mesh_element.appendChild(bsdf_element)
        self.scene.appendChild(mesh_element)
        
        mesh.select_set(False)

class NoriExporter(bpy.types.Operator, ExportHelper):
    bl_idname = "export_scene.nori"
    bl_label = "Export Nori scene"
    filename_ext = ".xml"
    filter_glob: StringProperty(default="*.xml", options={'HIDDEN'})

    def execute(self, context):
        # Ensure we are in object mode
        if context.active_object and context.active_object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
            
        writer = NoriWriter(context, self.filepath)
        writer.write()
        return {'FINISHED'}

def menu_func_export(self, context):
    self.layout.operator(NoriExporter.bl_idname, text="Export Nori scene (.xml)")

def register():
    bpy.utils.register_class(NoriExporter)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(NoriExporter)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()