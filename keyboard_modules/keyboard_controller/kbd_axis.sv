module kbd_axis #(
	parameter int AXIS_DATA_WIDTH = 8, // PS/2 scancode is 8 bits
	parameter int DEBOUNCE_COUNTER_SIZE = 8,
	parameter int CLK_FREQ = 50000000
) (
	// physical PS/2 pins (connect from top-level pins or gpio)
	input  wire                ps2_clk_i,
	input  wire                ps2_data_i,
	// AXI stream interface signals
	input  wire                axis_aclk_i,
	input  wire                axis_aresetn_i,
	// master axi stream interface
	input wire m_axis_tready_i,
	output wire m_axis_tvalid_o,
	output reg [AXIS_DATA_WIDTH - 1:0] m_axis_tdata_o
);

	// instantiate the existing VHDL PS/2 front-end (ps2_keyboard.vhd)
	// It produces ps2_code_new (one-cycle pulse) and ps2_code (8-bit)
	wire ps2_code_new;
	wire [7:0] ps2_code;

	ps2_keyboard #(
		.debounce_counter_size(DEBOUNCE_COUNTER_SIZE),
		.clk_freq(CLK_FREQ)
	) ps2_keyboard_inst (
		.clk(axis_aclk_i),
		.ps2_clk(ps2_clk_i),
		.ps2_data(ps2_data_i),
		.ps2_code_new(ps2_code_new),
		.ps2_code(ps2_code)
	);

	// AXIS producer state
	reg tvalid_reg;
	assign m_axis_tvalid_o = tvalid_reg;

	typedef enum logic {IDLE, SEND} state_t;
	state_t state;

	// simple producer: when PS/2 front-end pulses ps2_code_new, present ps2_code on TDATA
	always @(posedge axis_aclk_i or negedge axis_aresetn_i) begin
		if (!axis_aresetn_i) begin
			tvalid_reg <= 1'b0;
			m_axis_tdata_o <= {AXIS_DATA_WIDTH{1'b0}};
			state <= IDLE;
		end else begin
			case (state)
				IDLE: begin
					tvalid_reg <= 1'b0;
					if (ps2_code_new) begin
						m_axis_tdata_o <= ps2_code;
						tvalid_reg <= 1'b1;
						state <= SEND;
					end
				end
				SEND: begin
					if (m_axis_tready_i) begin
						tvalid_reg <= 1'b0;
						state <= IDLE;
					end
				end
			endcase
		end
	end

endmodule
