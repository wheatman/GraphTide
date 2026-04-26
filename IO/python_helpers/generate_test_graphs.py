import struct
import os
import random

# Map enum names to binary DataType codes (must match C++ side)
DATA_TYPE_CODES = {
  'NONE' : 0,
  'UINT8' : 10,
  'UINT16' : 11,
  'UINT32' : 12,
  'INT8' : 20,
  'INT16' : 21,
  'INT32' : 22,
  'INT64' : 23,
  'FLOAT32' : 30,
  'FLOAT64' : 31
}

def get_random_value(type):
    if type == 'UINT8':
        return random.randrange(0, 1 << 8)
    if type == 'UINT16':
        return random.randrange(0, 1 << 16)
    if type == 'UINT32':
        return random.randrange(0, 1 << 32)
    if type == 'INT8':
        return random.randrange(-(1 << 7), 1 << 7)
    if type == 'INT16':
        return random.randrange(-(1 << 15), 1 << 15)
    if type == 'INT32':
        return random.randrange(-(1 << 31), 1 << 31)
    if type == 'INT64':
        return random.randrange(-(1 << 63), 1 << 63)
    if type == "FLOAT32":
        return random.random()
    if type == "FLOAT64":
        return random.random()
    
def write_type(f, type):
    value = get_random_value(type)
    if type == 'UINT8':
        f.write(struct.pack('<B', value)) 
    if type == 'UINT16':
        f.write(struct.pack('<H', value)) 
    if type == 'UINT32':
        f.write(struct.pack('<I', value)) 
    if type == 'INT8':
        f.write(struct.pack('<b', value)) 
    if type == 'INT16':
        f.write(struct.pack('<h', value)) 
    if type == 'INT32':
        f.write(struct.pack('<i', value)) 
    if type == 'INT64':
        f.write(struct.pack('<q', value)) 
    if type == "FLOAT32":
        f.write(struct.pack('<f', value)) 
    if type == "FLOAT64":
        f.write(struct.pack('<d', value)) 
    

# Directory to store output test files
os.makedirs("test_graphs", exist_ok=True)

def write_graph_header(filename,
                       num_vertices,
                       num_unique_edges,
                       total_updates,
                       if_signal_graph,
                       vertex_id_type,
                       timestamp_type,
                       if_directed,
                       weight_type):
    with open(filename, 'wb') as f:
        f.write(struct.pack('<Q', num_vertices))        # uint64_t
        f.write(struct.pack('<Q', num_unique_edges))    # uint64_t
        f.write(struct.pack('<Q', total_updates))       # uint64_t
        f.write(struct.pack('<B', int(if_signal_graph)))# 1-byte bool
        f.write(struct.pack('<B', DATA_TYPE_CODES[vertex_id_type]))  # vertex_id_type
        f.write(struct.pack('<B', DATA_TYPE_CODES[timestamp_type]))  # timestamp_type
        f.write(struct.pack('<B', int(if_directed)))    # 1-byte bool
        f.write(struct.pack('<B', DATA_TYPE_CODES[weight_type]))     # weight_type
        f.write(b'\x00' * (32 - 8*3 - 5))  # padding to 32 bytes
        for i in range(total_updates):
            write_type(f, vertex_id_type)
            write_type(f, vertex_id_type)
            if timestamp_type != "NONE":
                write_type(f, timestamp_type)
            if weight_type != "NONE":
                write_type(f, weight_type)
            if if_signal_graph == False:
                if i % 2:
                    f.write(struct.pack('<B', 0)) 
                else:
                    f.write(struct.pack('<B', 1)) 

        

# Generate a few diverse test cases
test_cases = [
    (256, 100, 2000000, False, 'UINT32', 'UINT16', True,  'UINT32')
    # (42,   99,   2, True,  'UINT16', 'UINT32', False, 'FLOAT64'),
    # (10,   10,   3, False, 'UINT32', 'INT8', False, 'UINT32'),
    # (1,    1,    4, True,  'INT64', 'NONE', True,  'INT64'),
    # (0,    0,    5, False, 'UINT32', 'FLOAT32', True,  'NONE'),  
]

# Write the files
for i, args in enumerate(test_cases):
    filename = f"test_graphs/graph_header_test_{i}.bin"
    write_graph_header(filename, *args)
    print(f"Wrote {filename}")
