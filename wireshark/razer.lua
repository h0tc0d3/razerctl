local razer_proto_info = {
    version = "1.0.0",
    author = "Grigory Vasilyev",
    repository = "https://github.com/h0tc0d3/razerctl"
}

set_plugin_info(razer_proto_info)

razer_proto = Proto("Razer", "Razer Protocol")

local req_types = {
    [0x00] = "REQUEST",
    [0x01] = "BUSY",
    [0x02] = "SUCCESSFUL",
    [0x03] = "FAILURE",
    [0x04] = "TIMEOUT",
    [0x05] = "NOT_SUPPORTED"
}

local rf_status = ProtoField.uint8("razer.status", "Request Status", base.HEX, req_types)                  -- 1b
local rf_id = ProtoField.uint8("razer.id", "ID", base.HEX)                                                 -- 1b Possibly I2C range
local rf_remaining_packets = ProtoField.uint16("razer.remaining_packets", "Remaining Packets", base.DEC)   -- 2b
local rf_protocol_type = ProtoField.uint8("razer.protocol_type", "Protocol Type", base.HEX)                -- 1b
local rf_args_bytes = ProtoField.uint8("razer.args_bytes", "Bytes", base.HEX) -- 1b Number of bytes after command byte
local rf_command_class = ProtoField.uint8("razer.command_class", "Command Class", base.HEX)                -- 1b Command byte
local rf_command_id = ProtoField.uint8("razer.command_id", "Command ID", base.HEX)                         -- 1b Sub Command byte
local rf_args = ProtoField.bytes("razer.args", "Arguments", base.SPACE)       -- 80b Params
local rf_crc = ProtoField.uint8("razer.crc", "CRC", base.HEX)                                              -- 1b CRC checksum                                  -- 1b End marker

razer_proto.fields = {
    rf_status,
    rf_id,
    rf_remaining_packets,
    rf_protocol_type,
    rf_args_bytes,
    rf_command_class,
    rf_command_id,
    rf_args,
    rf_crc
}

local data_direction = Field.new("usb.endpoint_address.direction") -- 1 IN, 0 OUT
local data_length = Field.new("usb.data_len")                     -- should be 90
local urb_length = Field.new("usb.urb_len")                       -- should be 90

function razer_proto.init()
    print("Initialization of Razer dissector")
end

function razer_proto.dissector(buffer, pinfo, tree)

    local blength = buffer:reported_length_remaining()
    if blength == 0 then
        return
    end

    if (data_length().value == 90 and urb_length().value == 90) then

        pinfo.cols["protocol"]:set("RAZER")

        if (data_direction().value == 0) then
            pinfo.cols["info"]:set("request")
        else
            pinfo.cols["info"]:set("response")
        end

        -- seek to the last 90 bytes
        local offset = buffer:len() - 90
        local t_razer = tree:add(razer_proto, "Razer Data")

        -- Status
        local status = offset
        offset = offset + 1
        -- ID
        t_razer:add(rf_id, buffer(offset, 1))
        offset = offset + 1
        t_razer:add(rf_status, buffer(status, 1))
        -- Remaining packets
        local remaning = offset
        offset = offset + 2
        -- Protocol Type
        t_razer:add(rf_protocol_type, buffer(offset, 1))
        offset = offset + 1
        -- Number of parameters bytes
        local num_params = buffer(offset, 1):le_uint()
        local params = offset
        offset = offset + 1
        -- Command class
        t_razer:add(rf_command_class, buffer(offset, 1))
        offset = offset + 1
        -- Command ID
        t_razer:add(rf_command_id, buffer(offset, 1))
        offset = offset + 1
        -- Command params
        t_razer:add(rf_args_bytes, buffer(params, 1))
        t_razer:add(rf_args, buffer(offset, num_params))
        offset = offset + 80 -- skip to the end
        -- CRC
        t_razer:add(rf_crc, buffer(offset, 1))
        offset = offset + 1
        t_razer:add(rf_remaining_packets, buffer(remaning, 2))

    end

end

register_postdissector(razer_proto)
--DissectorTable.get("usb.control"):add(0xffff, razer_proto)
