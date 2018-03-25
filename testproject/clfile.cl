__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;



__kernel void motion_comp_local(__read_only image2d_t a, __read_only image2d_t b, 
							 int2 __global *motionVec, __write_only image2d_t output) {
	const int WS = 8;
	const int X = get_group_id(0);
	const int Y = get_group_id(1);

	const int block_x = X * WS + WS;
	const int block_y = Y * WS + WS;
	
	const int f  =  get_local_id(0);
	const int f1 =  get_local_id(1);

	const int width = get_image_width(a);


	__local float4 A[WS][WS];
	__local float4 B[WS*3][WS*3];
	A[f][f1] = read_imagef(a, sampler, (int2)(block_x+f1, block_y+f));
	
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {	
			B[(i+1)*WS+f][(j+1)*WS+f1] = read_imagef(b, sampler, (int2)(block_x+f1 + (j*WS), block_y+f + (i*WS)));
		}
	}  
	barrier(CLK_LOCAL_MEM_FENCE);

	__local int mn;
	mn = 200000000;
	for (int cur_i = 0; cur_i <= 16; cur_i++){
		 for (int cur_j = 0; cur_j <= 16; cur_j++) {
			__private int sum;
			sum = 0;
			__private float4 pix1 = A[f][f1];
			__private float4 pix2 = B[cur_i+f][cur_j+f1];
			sum += (int)((pix1.x - pix2.x)*(pix1.x - pix2.x));
			barrier(CLK_LOCAL_MEM_FENCE);
			if (sum < mn) {
				mn = sum;
				int idx = Y * ((width-WS*2)/WS) + X;
				motionVec[idx] = (int2)(cur_i-WS, cur_j-WS);
			}
			
		}
	}
}







__kernel void motion_comp_global(__read_only image2d_t a, __read_only image2d_t b, 
							 int2 __global *motionVec, __write_only image2d_t output) {
	const int WS = 8;
	const int X = get_global_id(0);
	const int Y = get_global_id(1);

	const int block_x = X * WS + WS;
	const int block_y = Y * WS + WS;

	const int width = get_image_width(a);
	//x - width
	//y - height
	
	int mn = 2000000000;
	 for (int cur_y = block_y - WS; cur_y <= block_y + WS; cur_y++) {
		for (int cur_x = block_x - WS; cur_x <= block_x + WS;  cur_x++){
			int sum = 0;
			for (int i = 0; i < WS; i++) {
				for (int j = 0; j < WS; j++) {
					float4 p1 = read_imagef(a, sampler, (int2)(block_x+i, block_y+j));
					float4 p2 = read_imagef(b, sampler, (int2)(cur_x+i, cur_y+j));
					sum += (int)((p1.x - p2.x)*(p1.x - p2.x));
				}
			}
			if (sum < mn) {
				mn = sum;
				int idx = Y * ((width-WS*2)/WS) + X;
				motionVec[idx] = (int2)(cur_y - block_y, cur_x - block_x);
			}
		}
	}


//	for (int i = 0; i < WS; i++) {
//		for (int j = 0; j < WS; j++) {
//			float4 px1 = read_imagef(a, sampler, (int2)(block_x+i, block_y+j));
//			//float4 px2 = read_imagef(b, sampler, (int2)(block_x+i, block_y+j));
//			write_imagef(output, (int2)(block_x+i, block_y+j), px1);
//		}
//	}
	
}
