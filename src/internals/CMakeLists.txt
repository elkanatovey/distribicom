set(proto_names boot distribicom)
foreach(proto_name ${proto_names})
    # Get proto files
    get_filename_component(proto "./protos/${proto_name}.proto" ABSOLUTE)
    get_filename_component(proto_dir "${proto}" DIRECTORY)
    # Generate source files
    set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
    set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")
    add_custom_command(
            OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
            -I "${proto_dir}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${proto}"
            DEPENDS "${proto}"
    )

    # hw_grpc_proto
    add_library(${proto_name}_grpc
            ${grpc_srcs}
            ${grpc_hdrs}
            ${proto_srcs}
            ${proto_hdrs}
            "${proto}")

    # Include generated *.pb.h files
    target_include_directories(${proto_name}_grpc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(${proto_name}_grpc
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})


endforeach()

