module multibyte_uart_rx 
#(
    parameter SYSTEMCLOCK           = 100_000_000, 
    parameter BAUDRATE              = 115_200, 
    parameter ELEMENT_WIDTH         = 8, 
    parameter NUMBER_OF_ELEMENTS    = 256, 
    localparam DATA_LENGTH_COUNTER_WIDTH = $clog2(NUMBER_OF_ELEMENTS)
)(
    input  logic                                             clk,
    input  logic                                             rst,
    input  logic                                             rx_line,
    input  logic                                             rx_empty_buffer,

    output logic [NUMBER_OF_ELEMENTS-1:0][ELEMENT_WIDTH-1:0] rx_data,
    output logic [DATA_LENGTH_COUNTER_WIDTH:0]               rx_data_length,
    output logic                                             rx_updated_buffer,
    output logic                                             rx_overflow
);

    logic [ELEMENT_WIDTH-1:0]   uart_rx_data;
    logic                       uart_rx_data_valid;
    
    uart_rx receiver (
        .clk(clk),
        .rst(rst),
        .rx_line(rx_line),
        .rx_data(uart_rx_data),
        .rx_data_valid(uart_rx_data_valid)
    );
    defparam receiver.BAUDRATE      = BAUDRATE;
    defparam receiver.SYSTEMCLOCK   = SYSTEMCLOCK;
    defparam receiver.ELEMENT_WIDTH = ELEMENT_WIDTH;
    
    always @(posedge clk) begin
    
        if (rst || rx_empty_buffer) begin
            rx_data_length    <= 'd0;
            rx_data           <= 'b0;
            rx_updated_buffer <= 'b0;
            rx_overflow       <= 'b0;
            
        end else begin
            rx_data_length    <= rx_data_length;
            rx_data           <= rx_data;
            rx_updated_buffer <= 'b0;
            rx_overflow       <= rx_overflow;
            
            if (uart_rx_data_valid) begin
                if (rx_data_length < NUMBER_OF_ELEMENTS) begin
                    rx_data[rx_data_length] <= uart_rx_data;
                    rx_data_length          <= rx_data_length + 1;
                    rx_updated_buffer       <= 'b1;
                end else begin
                    rx_overflow             <= 'b1;
                end
            end
    
        end
    end
endmodule