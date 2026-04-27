#include <arpa/inet.h>
#include <ctaps.h>
#include <stdio.h>
#include <string.h>

void respond_and_close_on_message_received(ct_connection_t* connection, 
                              ct_message_t* received_message,
                              ct_message_context_t* message_context) {

    uint16_t port = ct_local_endpoint_get_resolved_port(
        ct_message_context_get_local_endpoint(message_context)
    );

    printf("Received message: %s on port %d\n", ct_message_get_content(received_message), port);
    // For TCP this will initiate teardown,
    // for QUIC this will send a FIN flag
    // and initiate teardown if the remote
    // has also sent a FIN
    ct_connection_close(connection);
}

void ping_on_ready(ct_connection_t* connection) {
    int num_connections = ct_connection_get_total_num_grouped_connections(connection);

    ct_message_t* message = 0;
    if (num_connections == 1) {
        printf("Original connection successfully connected to server\n");
        char* msg = "hello from original";
        message = ct_message_new_with_content(msg,
                                              strlen(msg) + 1);
        // Connection cloning is compatible with both TCP
        // and QUIC, but will utilize multistreaming if
        // QUIC is the underlying protocol and create a new
        // TCP connection if TCP is the underlying protocol.
        //
        // This way the application gain advantages from
        // multistreaming when available, without worrying
        // about compatibility
        ct_connection_clone(connection);
    }
    else {
        printf("Clone successfully connected to server\n");
        char* msg = "hello from clone";
        message = ct_message_new_with_content(msg,
                                              strlen(msg) + 1);
    }
    ct_send_message(connection, message);
    // CTaps takes a deep copy, so the message can be freed immediately
    ct_message_free(message);

    ct_receive_callbacks_t receive_message_request = {
        .receive_callback = respond_and_close_on_message_received,
    };

    ct_receive_message(connection, &receive_message_request);
}

void free_on_connection_closed(ct_connection_t* connection) {
    ct_connection_free(connection);
}

int main() {
    // Needed to initialize internal CTaps event loop
    ct_initialize();

    ct_set_log_level(CT_LOG_ERROR);

    ct_remote_endpoint_t* remote_endpoint = ct_remote_endpoint_new();
    ct_remote_endpoint_with_ipv4(remote_endpoint, inet_addr("127.0.0.1"));
    ct_remote_endpoint_with_port(remote_endpoint, 1234);

    ct_local_endpoint_t* local = ct_local_endpoint_new();
    // Only attempt connections from localhost!
    ct_local_endpoint_with_interface(local, "lo");

    ct_transport_properties_t* transport_properties = ct_transport_properties_new();
    ct_transport_properties_set_reliability(transport_properties, REQUIRE);
    // Prefer QUIC, but remain compatible with TCP
    ct_transport_properties_set_preserve_msg_boundaries(transport_properties, PREFER);
    ct_transport_properties_set_multistreaming(transport_properties, PREFER);

    // Security parameters are required for QUIC,
    // but are ignored if TCP wins the race.
    ct_security_parameters_t* security_parameters = ct_security_parameters_new();

    ct_security_parameters_add_alpn(security_parameters, "example");
    ct_security_parameters_add_client_certificate(security_parameters,
                                                  "resources/cert.pem",
                                                  "resources/key.pem");

    ct_preconnection_t* preconnection = ct_preconnection_new(
        &local, 1, &remote_endpoint, 1, transport_properties, security_parameters);

    ct_connection_callbacks_t connection_callbacks = {
        .ready = ping_on_ready,
        .closed = free_on_connection_closed
    };

    // This will initiate racing, attempt a QUIC connection
    // first, but attempt TCP after a stagger delay
    ct_preconnection_initiate(preconnection, &connection_callbacks);

    ct_start_event_loop();

    // CTaps takes deep copies of the passed
    // objects internally, so the application retains
    // ownership of the orignals
    ct_local_endpoint_free(local);
    ct_security_parameters_free(security_parameters);
    ct_preconnection_free(preconnection);
    ct_transport_properties_free(transport_properties);
    ct_remote_endpoint_free(remote_endpoint);

    // Needed to free internal event loop
    ct_close();
    return 0;
}
