module if_axis
(
	// external CPU interface signals
	input wire [31:0] addr_i,
	output wire [31:0] data_o,
	output wire data_access_o,
	input wire data_w_i,
	// AXI stream interface signals
	input wire axis_aclk_i,
	input wire axis_aresetn_i,
	// slave axi stream interface
	output wire s_axis_tready_o,
	input wire s_axis_tvalid_i,
	input wire [AXIS_DATA_WIDTH - 1:0] s_axis_tdata_i
);

	parameter [31:0] SOC_SEGMENT = 'He4;
	parameter [31:0] SOC_CLASS = 'Ha9;
	parameter [31:0] AXIS_DATA_WIDTH = 8;

	// AXIS internal signals
	wire data_access;
	reg [31:0] data_read_axis;
	reg [1:0] s_axis_control; wire [1:0] m_axis_control;
	wire [AXIS_DATA_WIDTH - 1:0] m_axis_data;
	wire [15:0] device;

	assign s_axis_tready_o = s_axis_control[0];
	assign device = addr_i[15:0];
	assign data_access = addr_i[31:24] == (SOC_SEGMENT) && addr_i[23:16] == (SOC_CLASS) ? 1'b1 : 1'b0;
	assign data_access_o = data_access;
	assign data_o = data_read_axis;

	// AXI memory mapped registers

	// Register read operations
	always @(posedge axis_aclk_i, negedge axis_aresetn_i) begin
		if (axis_aresetn_i == 1'b0) begin
			data_read_axis <= {32{1'b0}};
			s_axis_control[0] <= 1'b0;
		end else begin
			if ((data_access == 1'b1)) begin
				// AXI peripheral is at 0xe4000000
				case (device[6:4])
				3'b001 : begin			// AXI control/status		0xe4000010 (RO)
					data_read_axis <= {28'h0000000,2'b00,s_axis_control[1],s_axis_control[0]};
				end
				3'b010 : begin			// AXI slave data (in)		0xe4000020 (RO)
					if (s_axis_control[1] == 1'b1) begin
						data_read_axis <= {24'h000000,s_axis_tdata_i};
						s_axis_control[0] <= 1'b1;
					end
				end
				default : begin
					data_read_axis <= {32{1'b0}};
				end
			endcase
			end
			
			// AXI data valid, clear TREADY
			s_axis_control[1] = s_axis_tvalid_i;
			if ((s_axis_control[1] == 1'b1 && s_axis_control[0] == 1'b1)) begin
				s_axis_control[0] <= 1'b0;
			end
		end
	end

endmodule
