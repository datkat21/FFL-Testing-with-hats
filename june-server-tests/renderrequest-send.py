import struct
import sys

def read_fflstoredata(file_path):
    with open(file_path, 'rb') as file:
        return file.read(96)

def main():
    if len(sys.argv) < 4:
        print("Usage: python script.py <path_to_FFLStoreData_file> <resolution> <output_file>")
        return

    fflstoredata_file = sys.argv[1]
    resolution = int(sys.argv[2])
    output_file = sys.argv[3]
    expression = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    export_as_gltf = True if len(sys.argv) > 5 else False

    fflstoredata = read_fflstoredata(fflstoredata_file)

    #resolution = 1600
    tex_resolution = 512
    data_length = len(fflstoredata)
    view_type = 0  # face
    #expression = 0
    resource_type = 1
    shader_type = 0
    camera_rotate = [0, 0, 0]
    model_rotate = [0, 0, 0]
    background_color = [1, 1, 1, 0]
    verify_charinfo = True
    verify_crc16 = True
    light_enable = True
    clothes_color = -1

    # Ensure fflstoredata is exactly 96 bytes
    #fflstoredata = fflstoredata[:96] + b'\x00' * (96 - len(fflstoredata))

    struct_format = '96sHB?HhBBBBIhhhhhhBBBBBB???bBB'
    packed_data = struct.pack(
        struct_format,
        fflstoredata,         # data: 96s
        data_length,          # dataLength: H (uint16_t)
        0,                    # modelType: B (uint8_t)
        export_as_gltf,       # exportAsGLTF: ? (bool)
        resolution,           # resolution: H (uint16_t)
        tex_resolution,       # texResolution: h (int16_t)
        view_type,            # viewType: B (uint8_t)
        resource_type,        # resourceType: B (uint8_t)
        shader_type,          # shaderType: B (uint8_t)
        expression,           # expression: B (uint8_t)
        0,                    # expressionFlag: I (uint32_t)
        camera_rotate[0],     # cameraRotate.x: h (int16_t)
        camera_rotate[1],     # cameraRotate.y: h (int16_t)
        camera_rotate[2],     # cameraRotate.z: h (int16_t)
        model_rotate[0],      # modelRotate.x: h (int16_t)
        model_rotate[1],      # modelRotate.y: h (int16_t)
        model_rotate[2],      # modelRotate.z: h (int16_t)
        background_color[0],  # backgroundColor[0]: B (uint8_t)
        background_color[1],  # backgroundColor[1]: B (uint8_t)
        background_color[2],  # backgroundColor[2]: B (uint8_t)
        background_color[3],  # backgroundColor[3]: B (uint8_t)
        0,                    # aaMethod: B (uint8_t)
        0,                    # drawStageMode: B (uint8_t)
        verify_charinfo,      # verifyCharInfo: ? (bool)
        verify_crc16,         # verifyCRC16: ? (bool)
        light_enable,         # lightEnable: ? (bool)
        clothes_color,        # clothesColor: b (int8_t)
        0,                    # instanceCount: B (uint8_t)
        0                     # instanceRotationMode: B (uint8_t)
    )

    # Write the packed data to the output file
    with open(output_file, 'wb') as file:
        file.write(packed_data)

    # Print the packed data in a human-readable format (hexadecimal)
    #print("Packed Data (hex):", packed_data.hex())

if __name__ == "__main__":
    main()
