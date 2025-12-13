// Include MicroPython API.
#include "py/runtime.h"
#include "py/obj.h"
#include <stdint.h>
#include <stddef.h>
#include "fb_alloc.h"
#include <string.h>

uint8_t* BEM(int width, int height, int** events, int event_size, uint8_t* output_buffer) {
    // Step 1: Allocate temporary signed integer array for counting
    int* temp_bem = fb_alloc(width * height * sizeof(int), FB_ALLOC_NO_HINT);
    memset(temp_bem, 0, width * height * sizeof(int));
    
    // Step 2: Accumulate ON/OFF events with fixed 90° rotation and y-flip
    for (int i = 0; i < event_size; i++) {
        int x = events[i][5];
        int y = events[i][4];
        int type = events[i][0];
        
        // Apply 90° clockwise rotation: (x, y) -> (y, width - 1 - x)
        int rotated_x = y;
        int rotated_y = width - 1 - x;
        
        // Apply y-axis flip: (x, y) -> (x, height - 1 - y)
        int new_x = rotated_x;
        int new_y = height - 1 - rotated_y;
        
        if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) {
            int idx = new_y * width + new_x;
            
            if (type == 0) {
                temp_bem[idx]--;  // Decrement for negative/OFF events
            } else if (type == 1) {
                temp_bem[idx]++;  // Increment for positive/ON events
            }
        }
    }
    
    // Step 3: Convert to binary (0 or 1) - write directly to output_buffer
    for (int i = 0; i < width * height; i++) {
        output_buffer[i] = (temp_bem[i] != 0) ? 1 : 0;
    }
    
    // Step 4: Free temporary buffer
    fb_free();  // Frees the most recent allocation (temp_bem)
    
    return output_buffer;
}

// Modified function signature without rotation parameter
mp_obj_t py_bem(size_t n_args, const mp_obj_t *args) {
    // Extract the arguments: width, height, events, event_size, output_buffer
    mp_obj_t width_obj = args[0];
    mp_obj_t height_obj = args[1];
    mp_obj_t events_obj = args[2];
    mp_obj_t event_size_obj = args[3];
    mp_obj_t output_obj = args[4];  // The pre-allocated output buffer
    
    int width = mp_obj_get_int(width_obj);
    int height = mp_obj_get_int(height_obj);
    int event_size = mp_obj_get_int(event_size_obj);
    
    // Get the output buffer from numpy array
    mp_buffer_info_t output_bufinfo;
    mp_get_buffer_raise(output_obj, &output_bufinfo, MP_BUFFER_WRITE);
    uint8_t *output_buffer = (uint8_t *)output_bufinfo.buf;
    
    // Get buffer info from numpy array (events)
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(events_obj, &bufinfo, MP_BUFFER_READ);
    
    // events_obj should be a 2D array with shape (event_size, 6)
    uint16_t *event_data = (uint16_t *)bufinfo.buf;
    
    // Create int** array for BEM function
    int **events = m_new(int*, event_size);
    for (int i = 0; i < event_size; i++) {
        events[i] = m_new(int, 6);
        for (int j = 0; j < 6; j++) {
            events[i][j] = (int)event_data[i * 6 + j];
        }
    }
    
    // Call BEM without rotation parameter
    BEM(width, height, events, event_size, output_buffer);
    
    // Free the allocated event arrays
    for (int i = 0; i < event_size; i++) {
        m_del(int, events[i], 6);
    }
    m_del(int*, events, event_size);
    
    // Return None since we modified the buffer in-place
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_bem_obj, 5, 5, py_bem);  // 5 args now

static const mp_rom_map_elem_t bem_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bem) },
    { MP_ROM_QSTR(MP_QSTR_BEM), MP_ROM_PTR(&py_bem_obj) },
};

static MP_DEFINE_CONST_DICT(bem_globals, bem_globals_table);

// Define module object.
const mp_obj_module_t bem_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *) &bem_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_bem, bem_module);