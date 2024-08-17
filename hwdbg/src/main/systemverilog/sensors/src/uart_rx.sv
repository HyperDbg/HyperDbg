module uart_rx 
#(
    parameter SYSTEMCLOCK                = 100_000_000, 
    parameter BAUDRATE                   = 115_200,
    parameter ELEMENT_WIDTH              = 8,
    localparam ELEMENT_BIT_COUNTER_WIDTH = $clog2(ELEMENT_WIDTH),
    localparam CLOCK_COUNTER_WIDTH       = 32,
    localparam CLOCKS_PER_BAUD           = SYSTEMCLOCK / BAUDRATE
)(
    input  logic                     clk,
    input  logic                     rst,
    input  logic                     rx_line,
    output logic [ELEMENT_WIDTH-1:0] rx_data,
    output logic                     rx_data_valid
);

    typedef enum {
        STATE_IDLE, 
        STATE_START_BIT, 
        STATE_RECEIVE_ELEMENT,
        STATE_CHECK_ELEMENT_RECEIVED,
        STATE_STOP_BIT, 
        STATE_DONE
    } state_t;
    state_t current_state;
    
    logic [ELEMENT_BIT_COUNTER_WIDTH:0]   current_bit;
    logic [CLOCK_COUNTER_WIDTH-1:0]       clock_counter;
    logic                                 clocks_per_baud_reached;
    logic                                 half_clocks_per_baud_reached;

    assign clocks_per_baud_reached 		= (clock_counter >= CLOCKS_PER_BAUD);
    assign half_clocks_per_baud_reached = (clock_counter >= (CLOCKS_PER_BAUD / 2));
    assign rx_data 						= rx_data;
    assign rx_data_valid 			    = current_state == STATE_DONE;

    always_ff @(posedge clk) begin
        if (rst) begin
            current_state <= STATE_IDLE;
            current_bit   <= 'b0;
            rx_data       <= 'b0;
            clock_counter <= 'd0;
            
        end else begin
            current_state <= current_state;
            current_bit   <= current_bit;
            rx_data       <= rx_data;
            clock_counter <= clock_counter + 1;
            
            case (current_state)
            
                STATE_IDLE: begin
                    if (rx_line == 'b0) begin
                        current_state <= STATE_START_BIT;
                        clock_counter <= 'b0;
                    end
                end
                
                STATE_START_BIT: begin
                    if (half_clocks_per_baud_reached) begin
                        current_state <= STATE_RECEIVE_ELEMENT;
                        clock_counter <= 'b0;
                        current_bit   <= 'b0;
                    end
                end
                
                STATE_RECEIVE_ELEMENT: begin
                    if (clocks_per_baud_reached) begin
                        rx_data[current_bit]    <= rx_line;
                        current_bit             <= current_bit + 1;
                        clock_counter           <= 'd0;
                        current_state           <= STATE_CHECK_ELEMENT_RECEIVED;
                    end
                end
                
                STATE_CHECK_ELEMENT_RECEIVED: begin
                    if (current_bit >= ELEMENT_WIDTH) begin
                        current_state <= STATE_STOP_BIT;
                    end else begin
                        current_state <= STATE_RECEIVE_ELEMENT;
                    end
                end
                
                STATE_STOP_BIT: begin
                    if (clocks_per_baud_reached) begin
                        current_state <= STATE_DONE;
                        clock_counter <= 'd0;
                    end
                end
                
                STATE_DONE: begin
                    current_state <= STATE_IDLE;
                end
                
            endcase
        end
    end
    
endmodule