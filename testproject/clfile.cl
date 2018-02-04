__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void motion_comp(__read_only image2d_t a, __read_only image2d_t b, int __global *ansX, int __global *ansY,
							int rows, int cols, __write_only image2d_t output) {
	const int wS = 8;
	const int block_w = get_global_id(0)*8+8;
	const int block_h = get_global_id(1)*8+8;
	
	int ax, ay;

	for (int cur_w = block_w - wS; cur_w <= block_w + wS; cur_w++) {
		for (int cur_h = block_h - wS; cur_h <= block_h + wS; cur_h++) {
			int min = 10000000;
			int sum = 0;
			for (int i = 0; i < wS; i++) {
				for (int j = 0; j < wS; j++) {
					float4 pixelA = read_imagef(a, sampler, (int2)(block_w+i, block_h+j));
					float4 pixelB = read_imagef(b, sampler, (int2)(cur_w+i, cur_h+j));
					sum += (int)((pixelA.x-pixelB.x)*(pixelA.x-pixelB.x));
				}
			}
			if (sum < min) {
				min = sum;
				ax = cur_w;
				ay = cur_h;
				//save (block_w-cur_w)(block_h-cur_h)
			}
		}
	}

	int idx = get_global_id(0) * 2 + get_global_id(1);
	ansX[idx] = get_global_id(0);
	ansY[idx] = get_global_id(1);
//		for (int i = 0; i < 8; i++) {
//			for (int j = 0; j < 8; j++) {
//				pixelA = read_imagef(a, sampler, (int2)(block_i+i, block_j+j));
//				pixelB = read_imagef(b, sampler, (int2)(block_i+i, block_j+j));
//				write_imagef(output, (int2)(block_i+i,block_j+j), pixelA);
//			}
//		}
		
}