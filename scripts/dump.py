import os
import struct
import json
import bindict # Once-Human client-specific library

def read_nxpk_filenames(file_path):
    filenames = []
    
    with open(file_path, 'rb') as f:
        # Check ID string
        if f.read(4) != b'NXPK':
            raise ValueError("Invalid file format: ID string 'NXPK' not found")
        
        # Read number of files
        f.seek(4)
        files_count = struct.unpack('<I', f.read(4))[0]
        
        # Read entry offset
        f.seek(0x14)
        entry_offset = struct.unpack('<I', f.read(4))[0]
        
        # Calculate names offset
        names_offset = entry_offset + (files_count * 0x1c) + 0x10
        
        # Read filenames
        for _ in range(files_count):
            f.seek(names_offset)
            filename = b''
            while True:
                char = f.read(1)
                if char == b'\x00':
                    break
                filename += char
            filenames.append(filename.decode('utf-8'))
            names_offset = f.tell()
    
    return filenames

def bindict_to_obj(bd) -> object:
    obj = {}
    for key in bd.keys():
        obj[key] = bd[key]
    return obj

if __name__ == '__main__':
    output_dir = os.path.join(os.getcwd(), 'json_output')
    script_path = os.path.join(os.getcwd(), 'script.npk')
    filenames = read_nxpk_filenames(script_path)
    
    # Convert filenames to tuple of (python module import path, filename) (e.g. \\ -> ., removing any file extension)
    module_paths = [(os.path.splitext(filename)[0].replace('\\', '.'), filename) for filename in filenames]
    
    # For each module, try to import, dir(), and check if there are any instances of "bindict.bindict" in the module.
    for (module_path, filename) in module_paths:
        try:
            module = __import__(module_path, fromlist=[''])
            dir_output = dir(module)
            
            # Dump any vars in the module that are an instance of the bindict.bindict class
            for var_key in dir_output:
                if var_key.startswith('__'):
                    continue
                var = getattr(module, var_key)
                if isinstance(var, bindict.bindict):
                    print(f"Module {module_path} contains an instance of bindict.bindict - dumping")
                    
                    # ensure path exists
                    output_path = os.path.join(output_dir, filename + '_' + var_key + '.json')
                    os.makedirs(os.path.dirname(output_path), exist_ok=True)
                    
                    # dump to json
                    with open(output_path, 'w', encoding='utf8') as f:
                        f.write(json.dumps(bindict_to_obj(var), ensure_ascii=False, indent=4))

        except Exception as e:
            print(f"Error importing {module_path}: {e}")
            continue