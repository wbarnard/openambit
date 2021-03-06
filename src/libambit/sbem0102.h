/*
 * (C) Copyright 2014 Emil Ljungdahl
 *
 * This file is part of libambit.
 *
 * libambit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 */
#ifndef __SBEM0102_H__
#define __SBEM0102_H__

#include <stddef.h>
#include <stdint.h>
#include "libambit.h"
#include "debug.h"
#include "utils.h"

enum ambit3_fw_gen {
    AMBIT3_FW_GEN1,
    AMBIT3_FW_GEN2,
    AMBIT3_FW_GEN3,
};

typedef struct libambit_sbem0102_s {
    uint16_t chunk_size;
    ambit_object_t *ambit_object;
} libambit_sbem0102_t;

typedef struct libambit_sbem0102_data_s {
    uint8_t *data;
    size_t size;
    uint8_t *read_ptr;
} libambit_sbem0102_data_t;


/**
 * Initialize SBEM0102 object
 * \param object Object to intialize
 * \param ambit_object Corresponding ambit_object
 * \param chunk_size Maximum chunk size of messages
 * \return 0 on success, else -1
 */
int libambit_sbem0102_init(libambit_sbem0102_t *object, ambit_object_t *ambit_object, uint16_t chunk_size);

/**
 * Deinitialize SBEM0102 object
 * \object Object to deintialize
 * \return 0 on success, else -1
 */
int libambit_sbem0102_deinit(libambit_sbem0102_t *object);

/**
 * Write data to device in SBEM0102 format
 * \param object libambit object
 * \param command Command to send
 * \param data data objects to append to request
 * \return 0 on success, else -1
 */
int libambit_sbem0102_write(libambit_sbem0102_t *object, uint16_t command, libambit_sbem0102_data_t *data);

/**
 * Request read of complete SBEM0102 message from device
 * \param object libambit object
 * \param command Command to send
 * \param data_objects objects to append to request
 * \param reply_data Data in reply. After data in object has been used, user
 * should call libambit_sbem0102_data_free()
 * \return 0 on success, else -1
 */
int libambit_sbem0102_command_request(libambit_sbem0102_t *object, uint16_t command, libambit_sbem0102_data_t *data_objects, libambit_sbem0102_data_t *reply_data);

/**
 * Request read of complete SBEM0102 message from device, but without SBEM data
 * in request
 * \param object libambit object
 * \param command Command to send
 * \param data raw data to append to request
 * \param reply_data Data in reply. After data in object has been used, user
 * should call libambit_sbem0102_data_free()
 * \return 0 on success, else -1
 */
int libambit_sbem0102_command_request_raw(libambit_sbem0102_t *object, uint16_t command, uint8_t *data, size_t datalen, libambit_sbem0102_data_t *reply_data);

/**
 * Free content of data objects
 * NOTE! The object itself is not freed
 * \param data Data object of which to free data in
 */
void libambit_sbem0102_data_init(libambit_sbem0102_data_t *data);

/**
 * Free content of data objects
 * NOTE! The object itself is not freed
 * \param data Data object of which to free data in
 */
void libambit_sbem0102_data_free(libambit_sbem0102_data_t *data);

/**
 * Append id:data pair to data object
 * \param object Data object to append value to
 * \param id data id to add
 * \param data data buffer to add
 * \param datalen length of databuffer
 */
void libambit_sbem0102_data_add(libambit_sbem0102_data_t *object, uint8_t id, uint8_t *data, uint8_t datalen);

/**
 * Reset internal read iterator
 * \param object Object to iterate over
 */
static inline void libambit_sbem0102_data_reset(libambit_sbem0102_data_t *object)
{
    object->read_ptr = NULL;
}

/**
 * Get data id of current element in iteration
 * \param object Object to iterate over
 * \return Current elements id
 */
static inline uint8_t libambit_sbem0102_data_id(libambit_sbem0102_data_t *object)
{
    return object->read_ptr[0];
}

/**
 * Get data length of current element in iteration
 * \param object Object to iterate over
 * \return Current elements data length
 */
static inline uint32_t libambit_sbem0102_data_len(libambit_sbem0102_data_t *object)
{
    if(object->read_ptr[1] == 0xff) {
       return object->read_ptr[2] | (object->read_ptr[3] << 8) | (object->read_ptr[4] << 16) | (object->read_ptr[5] << 24);
    }
    return (uint8_t)object->read_ptr[1];
}

/**
 * Get data pointer of current element in iteration
 * \param object Object to iterate over
 * \return Current elements data pointer
 */
static inline const uint8_t *libambit_sbem0102_data_ptr(libambit_sbem0102_data_t *object)
{
    if(libambit_sbem0102_data_len(object)>254) {
        return &object->read_ptr[6];
    }

    return &object->read_ptr[2];
}


/**
 * Iterate to next data entry
 * \param object Object to iterate over
 * \return 0 on success, else -1
 */
static inline int libambit_sbem0102_data_next(libambit_sbem0102_data_t *object, enum ambit3_fw_gen fw_gen)
{
    size_t read_offset;
    uint8_t log_end[] = { 0, 0, 0, 0, 0x7a, 0x44 };
    uint8_t *data;

    // Initial state
    if (object->read_ptr == NULL) {
        object->read_ptr = object->data;
        return 0;
    }
    // Loop state
    switch (object->read_ptr[0]) {
      case 0x7a:
      case 0x8a:
        if(fw_gen == AMBIT3_FW_GEN2) {
            read_offset = (size_t) (object->read_ptr - object->data);
            data = find_sequence(object->read_ptr, object->size - read_offset,
                                 log_end, ARRAY_LENGTH(log_end));
            if (data) {
                object->read_ptr = data + 4;
                return 0;
            }
            else {
                return -1;
            }
            break;
        }
      default:
        if (object->data + object->size > (uint8_t*)libambit_sbem0102_data_ptr(object) + libambit_sbem0102_data_len(object)) {
            object->read_ptr = (uint8_t*)libambit_sbem0102_data_ptr(object) + libambit_sbem0102_data_len(object);
            return 0;
        }
        break;
    }
    // Exit state
    return -1;
}

#endif /* __SBEM0102_H__ */
