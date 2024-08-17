`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/17/2024 05:14:28 PM
// Design Name: 
// Module Name: ro_sensor
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module ro_sensor
  #(parameter  integer width_size = 64,
    localparam integer width_size_local = width_size)
    (
       input logic [width_size-1:0]ro_meas_time,    
       input logic clk,             
       input logic  r_out,           
       input logic ro_counter_en,           
       output logic [width_size-1:0]ro_meas_count           
    );
    
    
    logic [width_size-1:0] clk_temp_counter;
        logic [width_size-1:0] ro_temp_counter;

    logic measure_flag;
    
     // RO_Counter Circuit
 // State encoding
    typedef enum logic [1:0] {
        IDLE   = 2'b00,
        MEASURE = 2'b01,
        OUT_READY = 2'b10,
        RST_VALS = 2'b11

    } state_t;

    state_t current_state, next_state;
    initial next_state <= IDLE;
    // State transition logic
    always_ff @(negedge clk ) begin
            current_state <= next_state;
        end

    // Next state logic
    always @(posedge clk) begin
        case (current_state)
            IDLE: begin
            for (int i = 0; i < width_size; i++) begin
                clk_temp_counter[i]         = 1'b0;
            end
                if (ro_counter_en) begin
                    next_state = RST_VALS;
                end else begin
                    next_state = IDLE;
                end
            end
            
              RST_VALS: begin
                next_state = MEASURE;
            
            end
                      
            MEASURE: begin
            if (clk_temp_counter < ro_meas_time) begin
            clk_temp_counter = clk_temp_counter+ ('d1);      
            next_state = MEASURE;
            end else begin
            for (int i = 0; i < width_size; i++) begin
                clk_temp_counter[i]         = 1'b0;
            end
            next_state = OUT_READY;
            end
            end
            
            OUT_READY: begin
                next_state = IDLE;
            
            end

//            default: begin
//                next_state = IDLE;
//            end
        endcase
    end

    // Output logic
//    assign state = current_state;
    
    
    
   always @( posedge r_out  ) begin

       if(current_state==MEASURE) begin
       ro_temp_counter=ro_temp_counter+ ('d1);
            
            
            end else if(current_state==OUT_READY) begin
                ro_meas_count     = ro_temp_counter;
                
                            
            
            end else if(current_state==RST_VALS) begin

                ro_temp_counter        = 'h0000000000000000;
     
     
     end
     end
         
   
//        `define assign_reg(index,reg1_t,reg2_t) \
//   assign reg1_t[((index+1)*60)+7:((index)*60)+8]=reg2_t[index];
   

//   `assign_reg(0,aes_ct,fult_count)
//   `assign_reg(1,aes_ct,fult_count)
////   `assign_reg(2,aes_ct,fult_count)
////   `assign_reg(3,aes_ct,fult_count)
////   `assign_reg(4,aes_ct,fult_count)
////   `assign_reg(5,aes_ct,fult_count)
////   `assign_reg(6,aes_ct,fult_count)
////   `assign_reg(7,aes_ct,fult_count)


endmodule
