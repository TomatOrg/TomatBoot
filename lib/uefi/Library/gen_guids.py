#!/usr/bin/python

import os
import re

if __name__ == '__main__':
    with open('guids.c', 'w') as guids_file:
        guids_file.write(
"""/*
 * AUTO GENERATED FILE
 *
 * Just a quick way to generate all the guids that EDK2 defines
 */
 
#include <Uefi.h>
 
""")

        # Fixes to problem I am too lazy to fix the name convertion for
        # or that just have a different guid name
        guid_name_fixes = {
            'EFI_VT100_GUID': 'EFI_VT_100_GUID',
            'EFI_VT100_PLUS_GUID': 'EFI_VT_100_PLUS_GUID',
            'EFI_VTUTF8_GUID': 'EFI_VT_UTF8_GUID',
            'EFI_UART_DEVICE_PATH_GUID': 'DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL',
            'EFI_SIMPLE_TEXT_IN_PROTOCOL_GUID': 'EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID',
            'EFI_SIMPLE_TEXT_OUT_PROTOCOL_GUID': 'EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID',
            'EFI_ACPI10_TABLE_GUID': 'EFI_ACPI_TABLE_GUID',
            'EFI_ACPI20_TABLE_GUID': 'EFI_ACPI_20_TABLE_GUID',
            'EFI_FILE_INFO_GUID': 'EFI_FILE_INFO_ID',

            # Why edk2, just why
            'EFI_EVENT_EXIT_BOOT_SERVICES_GUID': 'EFI_EVENT_GROUP_EXIT_BOOT_SERVICES',
            'EFI_EVENT_VIRTUAL_ADDRESS_CHANGE_GUID': 'EFI_EVENT_GROUP_VIRTUAL_ADDRESS_CHANGE',
            'EFI_EVENT_MEMORY_MAP_CHANGE_GUID': 'EFI_EVENT_GROUP_MEMORY_MAP_CHANGE',
            'EFI_EVENT_READY_TO_BOOT_GUID': 'EFI_EVENT_GROUP_READY_TO_BOOT',
            'EFI_EVENT_DXE_DISPATCH_GUID': 'EFI_EVENT_GROUP_DXE_DISPATCH_GUID',
            'EFI_FILE_SYSTEM_INFO_GUID': 'EFI_FILE_SYSTEM_INFO_ID',
            'EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID_GUID': 'EFI_FILE_SYSTEM_VOLUME_LABEL_ID',
            'EFI_GLOBAL_VARIABLE_GUID': 'EFI_GLOBAL_VARIABLE',
            'EFI_PART_TYPE_SYSTEM_PART_GUID': 'EFI_PART_TYPE_EFI_SYSTEM_PART_GUID',
            'EFI_UNICODE_COLLATION2_PROTOCOL_GUID': 'EFI_UNICODE_COLLATION_PROTOCOL2_GUID',
            'EFI_DEBUG_PORT_PROTOCOL_GUID': 'EFI_DEBUGPORT_PROTOCOL_GUID',
            'EFI_DEBUG_PORT_VARIABLE_GUID': 'EFI_DEBUGPORT_VARIABLE_GUID',
            'EFI_DEBUG_PORT_DEVICE_PATH_gGdtPtrGUID': 'DEVICE_PATH_MESSAGING_DEBUGPORT',

            # yes
            'EFI_MP_SERVICE_PROTOCOL_GUID': 'EFI_MP_SERVICES_PROTOCOL_GUID',
        }

        # Go over all the files in the include folder
        for root,dirs,files in os.walk('../Include/'):
            for file in files:
                with open(os.path.join(root, file)) as f:

                    # Search for all the guids in the file
                    guids = re.findall("\s*extern\s+EFI_GUID\s+g([a-zA-Z0-9_]+);", f.read())

                    # check there are even guids in the file
                    if len(guids) > 0:

                        # Generate the include for the file
                        filename_include = os.path.join(root, file)[len('../Include/'):]
                        print(filename_include)
                        guids_file.write('#include <{}>\n'.format(filename_include))

                        # For each of the guids
                        for guid_var_name in guids:

                            # Transform to snake case
                            guid_name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', guid_var_name)
                            guid_name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', guid_name).upper()

                            # Do fix if needed
                            if guid_name in guid_name_fixes:
                                guid_name = guid_name_fixes[guid_name]

                            # Write it to the file
                            print(guid_name)
                            guids_file.write('EFI_GUID g{} = {};\n'.format(guid_var_name, guid_name))

                        guids_file.write('\n')