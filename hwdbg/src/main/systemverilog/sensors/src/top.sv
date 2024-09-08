`timescale 1ns/1ps

module UART_FF
(
    input   logic clk,
    input   logic rst,
    input   logic uart_rx_line,
    input   logic reg_clk,
    input   logic reg_eop,
//    input   logic SPI_nCS,
//    output  logic clk_reg_out,
    output  logic uart_tx_line,
//    output  logic laser_module_trigger,
    output  logic gpio,

    output  [3:0] led_out_ff

);



    localparam SYSTEMCLOCK                      = 100_000_000;
//        localparam SYSTEMCLOCK                      = 10_000_000;

    localparam UART_BAUDRATE                    = 115_200;
    localparam UART_ELEMENT_WIDTH               = 8;
    localparam MULTIBYTE_RX_NUMBER_OF_ELEMENTS  = 256;
    localparam MULTIBYTE_RX_COUNTER_WIDTH       = $clog2(MULTIBYTE_RX_NUMBER_OF_ELEMENTS);
    
    logic                                                               uart_rx_empty_buffer;
    logic [MULTIBYTE_RX_NUMBER_OF_ELEMENTS-1:0][UART_ELEMENT_WIDTH-1:0] uart_rx_data;
    logic [MULTIBYTE_RX_COUNTER_WIDTH:0]                                uart_rx_data_length;
    logic                                                               uart_rx_updated_buffer;
    logic                                                               uart_rx_overflow;
    multibyte_uart_rx multibyte_uart_rxer (
        .clk                    (clk),
        .rst                    (rst),
        .rx_line                (uart_rx_line),
        .rx_empty_buffer        (uart_rx_empty_buffer),
        .rx_data                (uart_rx_data),
        .rx_data_length         (uart_rx_data_length),
        .rx_updated_buffer      (uart_rx_updated_buffer),
        .rx_overflow            (uart_rx_overflow)
    );
    defparam multibyte_uart_rxer.SYSTEMCLOCK        = SYSTEMCLOCK;
    defparam multibyte_uart_rxer.BAUDRATE           = UART_BAUDRATE;
    defparam multibyte_uart_rxer.ELEMENT_WIDTH      = UART_ELEMENT_WIDTH;
    defparam multibyte_uart_rxer.NUMBER_OF_ELEMENTS = MULTIBYTE_RX_NUMBER_OF_ELEMENTS;
    



    localparam MULTIBYTE_TX_NUMBER_OF_ELEMENTS        = 256;
    localparam DELAY_COUNTER_WIDTH                    = 32;
    localparam MULTIBYTE_TX_COUNTER_WIDTH             = $clog2(MULTIBYTE_TX_NUMBER_OF_ELEMENTS);

    logic                                                               uart_tx_en;
    logic [MULTIBYTE_TX_NUMBER_OF_ELEMENTS-1:0][UART_ELEMENT_WIDTH-1:0] uart_tx_data;
    logic [MULTIBYTE_TX_COUNTER_WIDTH:0]                                uart_tx_data_length;
    logic [DELAY_COUNTER_WIDTH-1:0]                                     delay_between_tx;
    logic                                                               uart_tx_ready;
    
    multibyte_uart_tx multibyte_uart_txer (
        .clk                        (clk),
        .rst                        (rst),
        .tx_en                      (uart_tx_en),
        .tx_data                    (uart_tx_data),
        .tx_data_length             (uart_tx_data_length),
        .delay_between_tx           (delay_between_tx),
        .tx_line                    (uart_tx_line),
        .tx_ready                   (uart_tx_ready)
    );
    defparam multibyte_uart_txer.SYSTEMCLOCK               = SYSTEMCLOCK; 
    defparam multibyte_uart_txer.BAUDRATE                  = UART_BAUDRATE; 
    defparam multibyte_uart_txer.ELEMENT_WIDTH             = UART_ELEMENT_WIDTH;
    defparam multibyte_uart_txer.NUMBER_OF_ELEMENTS        = MULTIBYTE_TX_NUMBER_OF_ELEMENTS; 
    defparam multibyte_uart_txer.DELAY_WIDTH               = DELAY_COUNTER_WIDTH;

    
    localparam RO_MEAS_BITS     = 64;
    localparam AES_BITS         = 128;

    localparam RO_EMAS_BYTE             = $clog2(RO_MEAS_BITS);
    localparam CLK_INT_DIV_BITS     = 32;
    localparam CLK_FRC_DIV_BITS     = 32; 


    localparam RO_INST                   = 32;
    logic [RO_INST-1:0]                 ro_en;
    logic [RO_INST-1:0]                 ro_out;
    
    logic  [RO_MEAS_BITS-1:0] ro_meas_time;
    
    logic  [AES_BITS-1:0] aes_key;
    logic  [AES_BITS-1:0] aes_ptx;
    logic  [AES_BITS-1:0] aes_ctx;
    logic  [64-1:0]   capacitance;

    logic  [RO_INST-1:0][RO_MEAS_BITS-1:0] ro_meas_count;
//    logic  [RO_MEAS_BITS-1:0] ro_meas_count;

    logic     ro_counter_en;
    
    logic [CLK_INT_DIV_BITS-1:0]         div_int;
    logic [CLK_FRC_DIV_BITS-1:0]         div_frac;  
    localparam FF_INST                   = 4;
    
    localparam CHAIN_INST                = 32;
    logic [CHAIN_INST-1:0]                 chain_en;

    
    logic                               clk_target;
    logic                               hult_signal;
    logic [FF_INST-1:0]                 ff_write_en;
    logic [FF_INST-1:0]                 ff_in;
    logic [FF_INST-1:0]                 ff_out;

    logic [8-1:0]                       ff_id_reg;
    logic [8-1:0]                       ff_val_reg;
    
    logic [8-1:0]                       ro_id_reg;
    logic [8-1:0]                       ro_val_reg;
    
    logic [8-1:0]                       chain_id_reg;
    logic [8-1:0]                       chain_val_reg;
    
    logic [16-1:0]                      temp_reg;
    logic [16-1:0]                      vint_reg;
    logic [16-1:0]                      vaux_reg;

    initial clk_target <=clk;

// (* S = "TRUE" *) (* KEEP = "TRUE" *) ff_fanout ff[FF_INST-1:0](
//     .clk(clk_target),
//     .CE(ff_write_en),
//     .inp(ff_in),
//     .rst(reg_clk),
//     .res(ff_out)
//     );RO_INST

 (* S = "TRUE" *) (* KEEP = "TRUE" *) ff_fanout ff[FF_INST-1:0](
     .clk(clk_target),
     .CE(ff_write_en),
     .inp(ff_in),
//     .rst(reg_eop),
     .rst(1'b0),
     .res(ff_out)
     );

//Inverter Chain


    wire clk_out1;
    wire locked;
    wire chain_clk_out;
    wire chain_freq_out;
    wire clk_in1;
    
//    assign ro_enable =ro_en;
    assign clk_in1 = clk;
    assign gpio =  chain_freq_out;

 (* S = "TRUE" *) (* KEEP = "TRUE" *)  Chain chain[CHAIN_INST-1:0](
    .chain_en(chain_en),
    .in_data(chain_clk_out),
    .out_data(chain_freq_out)
    );
    
    




  clock_gen instance_name
   (
    // Clock out ports
    .clk_out1(clk_out1),     // output clk_out1
   // Clock in ports
    .clk_in1(clk_in1)      // input clk_in1
);
    
    wire [5 : 0] channel_out;
    wire eoc_out;
    
    system_management_wiz_0 temp_sensor (
  .di_in(16'b0),              // input wire [15 : 0] di_in
  .daddr_in({{2{1'b0}},channel_out}),        // input wire [7 : 0] daddr_in
  .den_in(eoc_out),            // input wire den_in
  .dwe_in(1'b0),            // input wire dwe_in
  .drdy_out(drdy_out),        // output wire drdy_out
  .do_out(temp_reg),            // output wire [15 : 0] do_out
  .dclk_in(clk_in1),          // input wire dclk_in
  .reset_in(1'b0),        // input wire reset_in
  .channel_out(channel_out),  // output wire [5 : 0] channel_out
  .eoc_out(eoc_out),          // output wire eoc_out
  .alarm_out(alarm_out),      // output wire alarm_out
  .eos_out(eos_out),          // output wire eos_out
  .busy_out(busy_out)        // output wire busy_out
);

    wire [5 : 0] channel_out_vint;
    wire eoc_out_vint;
    wire alarm_out_1,eos_out_1,busy_out_1,drdy_out_1;

system_management_wiz_1 v_int_sensor (
  .di_in(16'b0),              // input wire [15 : 0] di_in
  .daddr_in({{2{1'b0}},channel_out_vint}),        // input wire [7 : 0] daddr_in
  .den_in(eoc_out_vint),            // input wire den_in
  .dwe_in(1'b0),            // input wire dwe_in
  .drdy_out(drdy_out_1),        // output wire drdy_out
  .do_out(vint_reg),            // output wire [15 : 0] do_out
  .dclk_in(clk_in1),          // input wire dclk_in
  .reset_in(1'b0),        // input wire reset_in
  .channel_out(channel_out_vint),  // output wire [5 : 0] channel_out
  .eoc_out(eoc_out_vint),          // output wire eoc_out
  .alarm_out(alarm_out_1),      // output wire alarm_out
  .eos_out(eos_out_1),          // output wire eos_out
  .busy_out(busy_out_1)        // output wire busy_out
);

    wire [5 : 0] channel_out_vaux;
    wire eoc_out_vaux;
    wire alarm_out_2,eos_out_2,busy_out_2,drdy_out_2;
    
system_management_wiz_2 v_aux_sensor (
  .di_in(16'b0),              // input wire [15 : 0] di_in
  .daddr_in({{2{1'b0}},channel_out_vaux}),        // input wire [7 : 0] daddr_in
  .den_in(eoc_out_vaux),            // input wire den_in
  .dwe_in(1'b0),            // input wire dwe_in
  .drdy_out(drdy_out_2),        // output wire drdy_out
  .do_out(vaux_reg),            // output wire [15 : 0] do_out
  .dclk_in(clk_in1),          // input wire dclk_in
  .reset_in(1'b0),        // input wire reset_in
  .channel_out(channel_out_vaux),  // output wire [5 : 0] channel_out
  .eoc_out(eoc_out_vaux),          // output wire eoc_out
  .alarm_out(alarm_out_2),      // output wire alarm_out
  .eos_out(eos_out_2),          // output wire eos_out
  .busy_out(busy_out_2)        // output wire busy_out
);


//      clock_gen instance_name(
//    // Clock out ports
//    .clk_out1(clk_out1),     // output clk_out1
//    // Status and control signals
//    .reset(1'b0), // input reset
//    .locked(locked),       // output locked
//   // Clock in ports
//    .clk_in1(clk)); 
    
    
    freq_generator freq_gen_inst(
   .clk_in(clk_out1),        // Input clock
    .reset(1'b0),         // Reset signal
    .div_int(div_int), // Integer part of the division factor
    .div_frac(div_frac), // Fractional part of the division factor (in fixed-point format, e.g., 0.5 = 32'd50000000)
    .clk_out(chain_clk_out) 
    );
    
    
// (* S = "TRUE" *) (* KEEP = "TRUE" *) ff_fanout ff[FF_INST-1:0](
//     .clk(reg_clk),
//     .CE(ff_write_en),
//     .inp(ff_in),
//     .rst(reg_clk),
//     .res(ff_out)
//     );
     
// (* S = "TRUE" *) (* KEEP = "TRUE" *) ro_fanout RO[RO_INST-1:0](
//     .R_in(ro_en),
//     .R_out(ro_out)
//     );
     
     (* S = "TRUE" *) (* KEEP = "TRUE" *)    ro_core RO[RO_INST-1:0](
     
       .in_d           (ro_en),
       .wire_inj          (ro_out) 

   );
   
   
   

	// Instantiate the Unit Under Test (UUT)
//	aes_128 aes_ht_free (
//		.clk(clk), 
//		.state(aes_ptx), 
//		.key(aes_key), 
//		.out(aes_ctx)
//	);



	// Outputs
//	wire [127:0] out;
//	wire [63:0] Capacitance;

	// Instantiate the Unit Under Test (UUT)
//	top_aes aes_ht(
//		.clk(clk), 
//		.rst(1'b0), 
//		.state(aes_ptx), 
//		.key(aes_key), 
//		.out(aes_ctx), 
//		.Capacitance(capacitance)
//	);

	    
    
     
        (* S = "TRUE" *) (* KEEP = "TRUE" *)    ro_sensor #(.width_size(RO_MEAS_BITS))  RO_SENS[RO_INST-1:0] (
       .ro_meas_time    (ro_meas_time),
       .clk             (clk),
       .r_out           (ro_out),
       .ro_counter_en          (ro_counter_en), 
       .ro_meas_count           (ro_meas_count)
   );

//        (* S = "TRUE" *) (* KEEP = "TRUE" *)    ro_sensor #(.width_size(RO_MEAS_BITS))  RO_SENS (
//       .ro_meas_time    (ro_meas_time),
//       .clk             (clk),
//       .r_out           (ro_out[0]),
//       .ro_counter_en          (ro_counter_en), 
//       .ro_meas_count           (ro_meas_count)
//   );
   
     
     
       
   assign led_out_ff = ff_out;
//   assign led_out_ff[0] =  1'b1;
    //assign clk_reg_out = reg_clk;
    
    typedef enum {  
            STATE_INPUT,
            STATE_UNHULT_CLK,
            STATE_HULT_CLK,
            STATE_SET_FF_VAL,
            STATE_GET_FF_VAL,
            STATE_LOCK_VAL,
            STATE_SET_FF_ID,
            STATE_GET_FF_ID,
            //RO STATES
            STATE_SET_RO_VAL,
            STATE_GET_RO_VAL,
            STATE_SET_RO_ID,
            STATE_GET_RO_ID,
            STATE_SET_RO_MEAS_TIME,
            STATE_GET_RO_MEAS_TIME,
            STATE_GET_MEAS_RO,
            STATE_EXEC_MEAS_RO,
            STATE_EXEC_MEAS_RO_POST,
            //AES STATES
            STATE_SET_AES_PTX,
            STATE_GET_AES_PTX,
            STATE_SET_AES_KEY,
            STATE_GET_AES_KEY,   
            STATE_GET_AES_CTX,
            //CHAIN STATES
            STATE_ACTIVATE_CHAIN,
            STATE_DEACTIVATE_CHAIN,
            STATE_SET_CHAIN_FREQ_INT,
            STATE_GET_CHAIN_FREQ_INT,
            STATE_SET_CHAIN_FREQ_FRAC,
            STATE_GET_CHAIN_FREQ_FRAC,
//          STATE_GET_NUMBER_OF_NCS,
            STATE_SET_GPIO,
            STATE_UNSET_GPIO,
            STATE_TRANSITION_INPUT,
            
            STATE_GET_TEMP_REG,
            STATE_GET_VINT_REG,
            STATE_GET_VAUX_REG,

            //STATE CHAIN 
            STATE_SET_CHAIN_VAL,
            STATE_GET_CHAIN_VAL,
            STATE_SET_CHAIN_ID,
            STATE_GET_CHAIN_ID

    } state_t;
    state_t current_state;

    typedef enum logic[7:0] {
        CMD_UNHULT_CLK              =  7,
        CMD_HULT_CLK                =  8,
        CMD_SET_FF_VAL              =  9,
        CMD_GET_FF_VAL              = 10,
        CMD_SET_FF_ID               = 11,
        CMD_GET_FF_ID               = 12,
//        CMD_GET_NUMBER_OF_NCS       = 11,
        CMD_SET_GPIO                = 13,
        CMD_UNSET_GPIO              = 14,
        CMD_SET_RO_VAL              =  15,
        CMD_GET_RO_VAL              = 16,
        CMD_SET_RO_ID               = 17,
        CMD_GET_RO_ID               = 18,
        CMD_SET_RO_MEAS_TIME        = 19,
        CMD_GET_RO_MEAS_TIME        = 20,
        CMD_GET_MEAS_RO             = 21,
        CMD_EXEC_MEAS_RO            = 22,
        CMD_ACTIVATE_CHAIN          = 23,
        CMD_DEACTIVATE_CHAIN        = 24, 
        CMD_SET_CHAIN_FREQ_INT      = 25,
        CMD_GET_CHAIN_FREQ_INT      = 26,
        CMD_SET_CHAIN_FREQ_FRAC     = 27,
        CMD_GET_CHAIN_FREQ_FRAC     = 28,
        
        CMD_SET_AES_PTX             = 29,
        CMD_GET_AES_PTX             = 30,
        CMD_SET_AES_KEY             = 31,
        CMD_GET_AES_KEY             = 32,
        CMD_GET_AES_CTX             = 33,
//        CMD_AES_EXEC                = 34
        CMD_GET_TEMP_REG            = 34,
        CMD_GET_VINT_REG            = 35,
        CMD_GET_VAUX_REG            = 36,

        CMD_SET_CHAIN_VAL           = 37,
        CMD_GET_CHAIN_VAL           = 38,
        CMD_SET_CHAIN_ID            = 39,
        CMD_GET_CHAIN_ID            = 40
            
    } command_t;
    
//    localparam TRIGGER_ON_DEFAULT         = 'h00000010;
//    localparam TRIGGER_OFFSET_DEFAULT     = 'h00000010;
//    localparam NUMBER_OF_PULSES_DEFAULT   = 'h0CMD_GET_CHAIN_FREQ0000010;
//    localparam PULSE_HIGH_WIDTH_DEFAULT   = 'h00000010;
//    localparam PULSE_LOW_WIDTH_DEFAULT    = 'h00000010;
    localparam DELAY_BETWEEN_TX_DEFAULT   = 'h00000010;




    
     always@( hult_signal) begin
     if (hult_signal==1'b1)
     clk_target<=0;
     else
     clk_target<=clk;
    end


    always_ff @(posedge clk) begin

        if (rst) begin
            current_state               <= STATE_INPUT;
            hult_signal                  <= 'b0;
            uart_rx_empty_buffer        <= 'b0;
            delay_between_tx            <= DELAY_BETWEEN_TX_DEFAULT;
            uart_tx_en                  <= 'b0;
            for (int i = 0; i < MULTIBYTE_TX_NUMBER_OF_ELEMENTS; i++) begin
                uart_tx_data[i]         <= 'h00;
            end
            uart_tx_data_length         <= 'd0;
            
//            gpio                        <= 'b0;
//            gpio_led                    <= 'd0;
//            trigger_on                  <= TRIGGER_ON_DEFAULT;

        end else begin
            current_state               <= current_state;
            uart_rx_empty_buffer        <= 'b0;
            delay_between_tx            <= delay_between_tx;
            uart_tx_en                  <= 'b0;
            for (int i = 0; i < MULTIBYTE_TX_NUMBER_OF_ELEMENTS; i++) begin
                uart_tx_data[i]         <= uart_tx_data[i];
            end
            uart_tx_data_length         <= uart_tx_data_length;
            
            
            case (current_state)
                STATE_INPUT: begin
                    if (uart_rx_updated_buffer) begin
                        case (uart_rx_data[uart_rx_data_length-1])
                        
                        
                         CMD_UNHULT_CLK: begin
                                current_state           <= STATE_UNHULT_CLK;
                            end
                            
                            CMD_HULT_CLK: begin
                                current_state           <= STATE_HULT_CLK;
                            end

                            CMD_SET_FF_VAL: begin
                                current_state           <= STATE_SET_FF_VAL;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_FF_VAL: begin
                                current_state           <= STATE_GET_FF_VAL;
                            end

                            CMD_SET_FF_ID: begin
                                current_state           <= STATE_SET_FF_ID;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_FF_ID: begin
                                current_state           <= STATE_GET_FF_ID;
                            end
                            
                       
                            //CMD RO
                            CMD_SET_RO_VAL: begin
                                current_state           <= STATE_SET_RO_VAL;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_RO_VAL: begin
                                current_state           <= STATE_GET_RO_VAL;
                            end

                            CMD_SET_RO_ID: begin
                                current_state           <= STATE_SET_RO_ID;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_RO_ID: begin
                                current_state           <= STATE_GET_RO_ID;
                            end
                            
                            //CMD CHAIN
                           CMD_SET_CHAIN_VAL: begin
                                current_state           <= STATE_SET_CHAIN_VAL;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_CHAIN_VAL: begin
                                current_state           <= STATE_GET_CHAIN_VAL;
                            end

                            CMD_SET_CHAIN_ID: begin
                                current_state           <= STATE_SET_CHAIN_ID;
                                uart_rx_empty_buffer    <= 'b1;
                            end

                            CMD_GET_CHAIN_ID: begin
                                current_state           <= STATE_GET_CHAIN_ID;
                            end
                            
                                 
                            CMD_SET_RO_MEAS_TIME: begin
                                current_state           <= STATE_SET_RO_MEAS_TIME;
                                uart_rx_empty_buffer    <= 'b1;

                            end
                                              
                            CMD_GET_RO_MEAS_TIME: begin
                                current_state           <= STATE_GET_RO_MEAS_TIME;
                            end
                            
                            //CMD SENSORS
                            CMD_GET_TEMP_REG: begin
                                current_state           <= STATE_GET_TEMP_REG;
                            end
                            
                            CMD_GET_VINT_REG: begin
                                current_state           <= STATE_GET_VINT_REG;
                            end
                            
                            CMD_GET_VAUX_REG: begin
                                current_state           <= STATE_GET_VAUX_REG;
                            end
                                                        
                                        
                            CMD_GET_MEAS_RO: begin
                                current_state           <= STATE_GET_MEAS_RO;
                            end        
                            
                            CMD_EXEC_MEAS_RO: begin
                                current_state           <= STATE_EXEC_MEAS_RO;
                            end    
                                
                            CMD_ACTIVATE_CHAIN: begin
                                current_state           <= STATE_ACTIVATE_CHAIN;
                            end        
                            
                            CMD_DEACTIVATE_CHAIN: begin
                                current_state           <= STATE_DEACTIVATE_CHAIN;
                            end     
                            
                            CMD_SET_CHAIN_FREQ_INT: begin
                                current_state           <= STATE_SET_CHAIN_FREQ_INT;
                                uart_rx_empty_buffer    <= 'b1;

                            end      
                              
                            CMD_GET_CHAIN_FREQ_INT: begin
                                current_state           <= STATE_GET_CHAIN_FREQ_INT;
                            end    
                            
                            CMD_SET_CHAIN_FREQ_FRAC: begin
                                current_state           <= STATE_SET_CHAIN_FREQ_FRAC;
                                uart_rx_empty_buffer    <= 'b1;

                            end      
                              
                            CMD_GET_CHAIN_FREQ_FRAC: begin
                                current_state           <= STATE_GET_CHAIN_FREQ_FRAC;
                            end 
                            
                            //AES CMDs
                            
                             CMD_SET_AES_KEY: begin
                                current_state           <= STATE_SET_AES_KEY;
                                uart_rx_empty_buffer    <= 'b1;

                            end      
                              
                            CMD_GET_AES_KEY: begin
                                current_state           <= STATE_GET_AES_KEY;
                            end 
                            
                            CMD_SET_AES_PTX: begin
                                current_state           <= STATE_SET_AES_PTX;
                                uart_rx_empty_buffer    <= 'b1;

                            end      
                              
                            CMD_GET_AES_PTX: begin
                                current_state           <= STATE_GET_AES_PTX;
                            end 
                            
                            CMD_GET_AES_CTX: begin
                                current_state           <= STATE_GET_AES_CTX;
                            end 

        
        
        
                             

//                            CMD_GET_NUMBER_OF_NCS: begin
//                                current_state           <= STATE_GET_NUMBER_OF_NCS;
//                            end
                            
                            CMD_SET_GPIO: begin
                                current_state           <= STATE_SET_GPIO;

                            end

                            CMD_UNSET_GPIO: begin
                                current_state           <= STATE_UNSET_GPIO;
                            end

                            default: begin
                                current_state           <= STATE_TRANSITION_INPUT;
                            end
                        endcase
                    end
                end
                
                
                
                 STATE_UNHULT_CLK: begin
                        hult_signal                   <=0;
                        current_state               <= STATE_TRANSITION_INPUT;
                    
                end
                
                
                
                STATE_HULT_CLK: begin
                        hult_signal                   <=1;
                        current_state               <= STATE_TRANSITION_INPUT;              
                end
                
               
             
          
                 
                STATE_SET_FF_ID: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           ff_id_reg <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
               STATE_GET_FF_ID: begin
                    if (uart_tx_ready) begin
//                        uart_tx_data[3] <= trigger_offset[7:0];
//                        uart_tx_data[2] <= trigger_offset[15:8];
//                        uart_tx_data[1] <= trigger_offset[23:16];
                        uart_tx_data[0] <= ff_id_reg;
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
                //STATE SENSORS
                
                
               STATE_GET_TEMP_REG: begin
                    if (uart_tx_ready) begin
                        uart_tx_data[1] <= temp_reg[7:0];
                        uart_tx_data[0] <= temp_reg[15:8];
                        uart_tx_data_length <= 'd2;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
                
                
                 STATE_GET_VINT_REG: begin
                    if (uart_tx_ready) begin
                        uart_tx_data[1] <= vint_reg[7:0];
                        uart_tx_data[0] <= vint_reg[15:8];
                        uart_tx_data_length <= 'd2;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
                
                STATE_GET_VAUX_REG: begin
                    if (uart_tx_ready) begin
                        uart_tx_data[1] <= vaux_reg[7:0];
                        uart_tx_data[0] <= vaux_reg[15:8];
                        uart_tx_data_length <= 'd2;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
              
                STATE_SET_FF_VAL: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           ff_in[ff_id_reg] <= uart_rx_data[0]&1;
                            ff_write_en[ff_id_reg]<=1;
                            current_state     <= STATE_LOCK_VAL;
                        end
                    end
                end
                
                
                
                STATE_SET_RO_MEAS_TIME: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd8) begin
                           ro_meas_time[7:0] <= uart_rx_data[7];// data comes in big endian
                           ro_meas_time[15:8] <= uart_rx_data[6];
                           ro_meas_time[23:16] <= uart_rx_data[5];
                           ro_meas_time[31:24] <= uart_rx_data[4];
                           ro_meas_time[39:32] <= uart_rx_data[3];
                           ro_meas_time[47:40] <= uart_rx_data[2];
                           ro_meas_time[55:48] <= uart_rx_data[1];
                           ro_meas_time[63:56] <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                       STATE_GET_RO_MEAS_TIME: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[7]<= ro_meas_time[7:0];  // data comes in big endian
                            uart_tx_data[6]<=ro_meas_time[15:8] ;
                            uart_tx_data[5]<=ro_meas_time[23:16];
                             uart_tx_data[4]<=ro_meas_time[31:24];
                             uart_tx_data[3]<=ro_meas_time[39:32];
                             uart_tx_data[2]<=ro_meas_time[47:40];
                             uart_tx_data[1]<=ro_meas_time[55:48];
                             uart_tx_data[0]<=ro_meas_time[63:56];
                        uart_tx_data_length <= 'd8;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end  
                
                
                //AES  STATES
                                STATE_SET_AES_PTX: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd16) begin
                           aes_ptx[7:0] <= uart_rx_data[15];// data comes in big endian
                           aes_ptx[15:8] <= uart_rx_data[14];
                           aes_ptx[23:16] <= uart_rx_data[13];
                           aes_ptx[31:24] <= uart_rx_data[12];
                           aes_ptx[39:32] <= uart_rx_data[11];
                           aes_ptx[47:40] <= uart_rx_data[10];
                           aes_ptx[55:48] <= uart_rx_data[9];
                           aes_ptx[63:56] <= uart_rx_data[8];
                           aes_ptx[71:64] <= uart_rx_data[7];// data comes in big endian
                           aes_ptx[79:72] <= uart_rx_data[6];
                           aes_ptx[87:80] <= uart_rx_data[5];
                           aes_ptx[95:88] <= uart_rx_data[4];
                           aes_ptx[103:96] <= uart_rx_data[3];
                           aes_ptx[111:104] <= uart_rx_data[2];
                           aes_ptx[119:112] <= uart_rx_data[1];
                           aes_ptx[127:120] <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
                  STATE_GET_AES_PTX: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[15]<= aes_ptx[7:0];  // data comes in big endian
                            uart_tx_data[14]<=aes_ptx[15:8] ;
                            uart_tx_data[13]<=aes_ptx[23:16];
                             uart_tx_data[12]<=aes_ptx[31:24];
                             uart_tx_data[11]<=aes_ptx[39:32];
                             uart_tx_data[10]<=aes_ptx[47:40];
                             uart_tx_data[9]<=aes_ptx[55:48];
                             uart_tx_data[8]<=aes_ptx[63:56];
                           uart_tx_data[7]<= aes_ptx[71:64];  // data comes in big endian
                            uart_tx_data[6]<=aes_ptx[79:72] ;
                            uart_tx_data[5]<=aes_ptx[87:80];
                             uart_tx_data[4]<=aes_ptx[95:88];
                             uart_tx_data[3]<=aes_ptx[103:96];
                             uart_tx_data[2]<=aes_ptx[111:104];
                             uart_tx_data[1]<=aes_ptx[119:112];
                             uart_tx_data[0]<=aes_ptx[127:120];
                        uart_tx_data_length <= 'd16;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end  
                
                   STATE_SET_AES_KEY: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd16) begin
                           aes_key[7:0] <= uart_rx_data[15];// data comes in big endian
                           aes_key[15:8] <= uart_rx_data[14];
                           aes_key[23:16] <= uart_rx_data[13];
                           aes_key[31:24] <= uart_rx_data[12];
                           aes_key[39:32] <= uart_rx_data[11];
                           aes_key[47:40] <= uart_rx_data[10];
                           aes_key[55:48] <= uart_rx_data[9];
                           aes_key[63:56] <= uart_rx_data[8];
                           aes_key[71:64] <= uart_rx_data[7];// data comes in big endian
                           aes_key[79:72] <= uart_rx_data[6];
                           aes_key[87:80] <= uart_rx_data[5];
                           aes_key[95:88] <= uart_rx_data[4];
                           aes_key[103:96] <= uart_rx_data[3];
                           aes_key[111:104] <= uart_rx_data[2];
                           aes_key[119:112] <= uart_rx_data[1];
                           aes_key[127:120] <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
                  STATE_GET_AES_KEY: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[15]<= aes_key[7:0];  // data comes in big endian
                            uart_tx_data[14]<=aes_key[15:8] ;
                            uart_tx_data[13]<=aes_key[23:16];
                             uart_tx_data[12]<=aes_key[31:24];
                             uart_tx_data[11]<=aes_key[39:32];
                             uart_tx_data[10]<=aes_key[47:40];
                             uart_tx_data[9]<=aes_key[55:48];
                             uart_tx_data[8]<=aes_key[63:56];
                           uart_tx_data[7]<= aes_key[71:64];  // data comes in big endian
                            uart_tx_data[6]<=aes_key[79:72] ;
                            uart_tx_data[5]<=aes_key[87:80];
                             uart_tx_data[4]<=aes_key[95:88];
                             uart_tx_data[3]<=aes_key[103:96];
                             uart_tx_data[2]<=aes_key[111:104];
                             uart_tx_data[1]<=aes_key[119:112];
                             uart_tx_data[0]<=aes_key[127:120];
                        uart_tx_data_length <= 'd16;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end 
                
                
                    STATE_GET_AES_CTX: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[15]<= aes_ctx[7:0];  // data comes in big endian
                            uart_tx_data[14]<=aes_ctx[15:8] ;
                            uart_tx_data[13]<=aes_ctx[23:16];
                             uart_tx_data[12]<=aes_ctx[31:24];
                             uart_tx_data[11]<=aes_ctx[39:32];
                             uart_tx_data[10]<=aes_ctx[47:40];
                             uart_tx_data[9]<=aes_ctx[55:48];
                             uart_tx_data[8]<=aes_ctx[63:56];
                           uart_tx_data[7]<= aes_ctx[71:64];  // data comes in big endian
                            uart_tx_data[6]<=aes_ctx[79:72] ;
                            uart_tx_data[5]<=aes_ctx[87:80];
                             uart_tx_data[4]<=aes_ctx[95:88];
                             uart_tx_data[3]<=aes_ctx[103:96];
                             uart_tx_data[2]<=aes_ctx[111:104];
                             uart_tx_data[1]<=aes_ctx[119:112];
                             uart_tx_data[0]<=aes_ctx[127:120];
                        uart_tx_data_length <= 'd16;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end 
                
  
            
                   
                
               STATE_EXEC_MEAS_RO: begin
                       ro_counter_en        <= 1'b1;
                        current_state       <= STATE_EXEC_MEAS_RO_POST;                       
                    
               end                    
               STATE_EXEC_MEAS_RO_POST: begin
                       ro_counter_en        <= 1'b0;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    
               end    
                
                
                    STATE_GET_MEAS_RO: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[7]<= ro_meas_count[ro_id_reg][7:0];  // data comes in big endian
                            uart_tx_data[6]<=ro_meas_count[ro_id_reg][15:8] ;
                            uart_tx_data[5]<=ro_meas_count[ro_id_reg][23:16];
                             uart_tx_data[4]<=ro_meas_count[ro_id_reg][31:24];
                             uart_tx_data[3]<=ro_meas_count[ro_id_reg][39:32];
                             uart_tx_data[2]<=ro_meas_count[ro_id_reg][47:40];
                             uart_tx_data[1]<=ro_meas_count[ro_id_reg][55:48];
                             uart_tx_data[0]<=ro_meas_count[ro_id_reg][63:56];
                        uart_tx_data_length <= 'd8;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end    
                
                
//              STATE_GET_MEAS_RO: begin
//                    if (uart_tx_ready) begin
//                           uart_tx_data[7]<= ro_meas_count[7:0];  // data comes in big endian
//                            uart_tx_data[6]<=ro_meas_count[15:8] ;
//                            uart_tx_data[5]<=ro_meas_count[23:16];
//                             uart_tx_data[4]<=ro_meas_count[31:24];
//                             uart_tx_data[3]<=ro_meas_count[39:32];
//                             uart_tx_data[2]<=ro_meas_count[47:40];
//                             uart_tx_data[1]<=ro_meas_count[55:48];
//                             uart_tx_data[0]<=ro_meas_count[63:56];
//                        uart_tx_data_length <= 'd8;
//                        uart_tx_en          <= 'b1;
//                        current_state       <= STATE_TRANSITION_INPUT;                       
//                    end
//                end    
                
                STATE_ACTIVATE_CHAIN :begin
                
//                    chain_en<='b1;
                    current_state     <= STATE_TRANSITION_INPUT;
                            
                 end
                 
                 
                  STATE_DEACTIVATE_CHAIN :begin
                
//                    chain_en<='b0;
                    current_state     <= STATE_TRANSITION_INPUT;
                            
                 end
                 
                 
                STATE_SET_CHAIN_FREQ_INT: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd4) begin
                           div_int[7:0] <= uart_rx_data[3];// data comes in big endian
                           div_int[15:8] <= uart_rx_data[2];
                           div_int[23:16] <= uart_rx_data[1];
                           div_int[31:24] <= uart_rx_data[0];

                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                       STATE_GET_CHAIN_FREQ_INT: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[3]<= div_int[7:0];  // data comes in big endian
                            uart_tx_data[2]<=div_int[15:8] ;
                            uart_tx_data[1]<=div_int[23:16];
                             uart_tx_data[0]<=div_int[31:24];

                        uart_tx_data_length <= 'd4;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end  
               
               
                
                STATE_SET_CHAIN_FREQ_FRAC: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd4) begin
                           div_frac[7:0] <= uart_rx_data[3];// data comes in big endian
                           div_frac[15:8] <= uart_rx_data[2];
                           div_frac[23:16] <= uart_rx_data[1];
                           div_frac[31:24] <= uart_rx_data[0];

                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                       STATE_GET_CHAIN_FREQ_FRAC: begin
                    if (uart_tx_ready) begin
                           uart_tx_data[3]<= div_frac[7:0];  // data comes in big endian
                            uart_tx_data[2]<=div_frac[15:8] ;
                            uart_tx_data[1]<=div_frac[23:16];
                             uart_tx_data[0]<=div_frac[31:24];

                        uart_tx_data_length <= 'd4;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;                       
                    end
                end  
                
                 
                 
                
                STATE_LOCK_VAL: begin
                   
                            ff_write_en[ff_id_reg]<=0;
                            current_state     <= STATE_TRANSITION_INPUT;
                   
                end  
                
                
                  
                                
                STATE_GET_FF_VAL: begin
                
                    if (uart_tx_ready) begin
                        uart_tx_data[0] <= {8{ff_in[ff_id_reg]}};
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
                
             
//                STATE_GET_NUMBER_OF_NCS: begin
//                    if (uart_tx_ready) begin
//                        uart_tx_data[3] <= number_of_ncs[7:0];
//                        uart_tx_data[2] <= number_of_ncs[15:8];
//                        uart_tx_data[1] <= number_of_ncs[23:16];
//                        uart_tx_data[0] <= number_of_ncs[31:24];
//                        uart_tx_data_length <= 'd4;
//                        uart_tx_en          <= 'b1;
//                        current_state       <= STATE_TRANSITION_INPUT;
//                    end
//                end
                
                STATE_SET_GPIO: begin
                  //  gpio            <= 'b1;
                    current_state   <= STATE_TRANSITION_INPUT;
                end
                
                
                                            
                 STATE_SET_RO_ID: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           ro_id_reg <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
               STATE_GET_RO_ID: begin
                    if (uart_tx_ready) begin
//                        uart_tx_data[3] <= trigger_offset[7:0];
//                        uart_tx_data[2] <= trigger_offset[15:8];
//                        uart_tx_data[1] <= trigger_offset[23:16];
                        uart_tx_data[0] <= ro_id_reg;
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
              
                STATE_SET_RO_VAL: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           ro_en[ro_id_reg] <= uart_rx_data[0]&1;
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
                  
                                
                STATE_GET_RO_VAL: begin
                    if (uart_tx_ready) begin
                        uart_tx_data[0] <= {8{ro_en[ro_id_reg]}};
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end         
                
                // CHAIN ID/VAL States
                
              STATE_SET_CHAIN_ID: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           chain_id_reg <= uart_rx_data[0];
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
               STATE_GET_CHAIN_ID: begin
                    if (uart_tx_ready) begin
//                        uart_tx_data[3] <= trigger_offset[7:0];
//                        uart_tx_data[2] <= trigger_offset[15:8];
//                        uart_tx_data[1] <= trigger_offset[23:16];
                        uart_tx_data[0] <= chain_id_reg;
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end
              
                STATE_SET_CHAIN_VAL: begin
                    if (uart_rx_updated_buffer) begin
                        if (uart_rx_data_length >= 'd1) begin
                           chain_en[chain_id_reg] <= uart_rx_data[0]&1;
                            current_state     <= STATE_TRANSITION_INPUT;
                        end
                    end
                end
                
                
                  
                                
                STATE_GET_CHAIN_VAL: begin
                    if (uart_tx_ready) begin
                        uart_tx_data[0] <= {8{chain_en[chain_id_reg]}};
                        uart_tx_data_length <= 'd1;
                        uart_tx_en          <= 'b1;
                        current_state       <= STATE_TRANSITION_INPUT;
                    end
                end  
                
                
          
          
          
                STATE_UNSET_GPIO: begin
                  //  gpio            <= 'b0;
                    current_state   <= STATE_TRANSITION_INPUT;
                end
                
                STATE_TRANSITION_INPUT: begin
                    uart_rx_empty_buffer    <= 'b1;
                    current_state           <= STATE_INPUT;
                end
            endcase
        end
    end
endmodule