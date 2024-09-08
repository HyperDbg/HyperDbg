module uart_tx 
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
    input  logic                     tx_en,
    input  logic [ELEMENT_WIDTH-1:0] tx_data,
    output logic                     tx_line,
    output logic                     tx_ready
);

    typedef enum {
        STATE_IDLE, 
        STATE_START_BIT, 
        STATE_TRANSFER_ELEMENT, 
        STATE_CHECK_ELEMENT_TRANSFERRED,
        STATE_STOP_BIT, 
        STATE_DONE
    } state_t;
    state_t current_state;

    logic [ELEMENT_BIT_COUNTER_WIDTH:0]     bit_counter    = 'b0;
    logic [CLOCK_COUNTER_WIDTH:0]           clock_counter  = 'b0;
    
    logic clocks_per_baud_reached;
    assign clocks_per_baud_reached = (clock_counter >= CLOCKS_PER_BAUD);

    always @(posedge clk) begin
    
        if (rst) begin
            current_state     <= STATE_IDLE;
            bit_counter       <= 'd0;
            clock_counter     <= 'd0;
            tx_line           <= 'b1;
            tx_ready          <= 'b0;
            
        end else begin
            
            clock_counter <= clock_counter + 1;
            current_state <= current_state;
            bit_counter   <= bit_counter;
            tx_line       <= tx_line;
            tx_ready      <= tx_ready;
            
            case (current_state)
            
                STATE_IDLE: begin
                    if (tx_en) begin
                        current_state <= STATE_START_BIT;
                        tx_line    <= 'b0;
                        tx_ready   <= 'b0;
                    end else begin
                        tx_line    <= 'b1;
                        tx_ready   <= 'b1;
                    end
                end
                
                STATE_START_BIT: begin
                        current_state <= STATE_TRANSFER_ELEMENT;
                        bit_counter   <= 'd0;
                        clock_counter <= 'd0;
                end
                
                STATE_TRANSFER_ELEMENT: begin
                    if(clocks_per_baud_reached) begin
                        clock_counter     <= 'b0;
                        tx_line           <= tx_data[bit_counter];
                        bit_counter       <= bit_counter + 1;
                        current_state     <= STATE_CHECK_ELEMENT_TRANSFERRED;
                    end
                end
                
                STATE_CHECK_ELEMENT_TRANSFERRED: begin
                    if (bit_counter >= ELEMENT_WIDTH) begin
                        current_state <= STATE_STOP_BIT;
                    end else begin
                        current_state <= STATE_TRANSFER_ELEMENT;
                    end
                end
                
                STATE_STOP_BIT: begin
                    if(clocks_per_baud_reached) begin
                        tx_line       <= 'b1;
                        clock_counter <= 'd0;
                        current_state <= STATE_DONE;
                    end
                end
                
                STATE_DONE: begin
                    if (clocks_per_baud_reached) begin
                        tx_ready         <= 'b1;
                        current_state <= STATE_IDLE;
                    end
                end
            endcase
        end
    end
endmodule