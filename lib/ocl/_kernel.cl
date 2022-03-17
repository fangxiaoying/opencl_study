__kernel void copy_data(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);

      int index =  (y * width + x) * 3;

      output_data[index]      = input_data[index];
      output_data[index + 1]  = input_data[index + 1];
      output_data[index + 2]  = input_data[index + 2];

}

__kernel void zero_copy(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      
      int index = mad24(y, width, x);

      uchar3 BGR = vload3(index, input_data);
      vstore3(BGR.xyz, index, output_data);

}

__kernel void BGRA2RGB(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      int index = mad24(y, width, x);

      uchar4 BGRA = vload4(index, input_data);
      vstore3(BGRA.zyx, index, output_data);
}


__kernel void vector(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      int index = mad24(y, width, x);
      // printf("%d ", index);

      uchar4 BGRA = vload4(index, input_data);
      printf("uc = %#v4hhx\n", BGRA);
      vstore3(BGRA.xyz, index, output_data);
}

__kernel void copy(__global const uchar *input_data,
                              __global uchar *output_data)
{

      int x = get_global_id(0);
      
      uchar16 in = vload16(x, input_data);
      vstore16(in, x, output_data);
}

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | 
                                    CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

__kernel void resize(read_only image2d_t src_image, 
                         write_only image2d_t dst_image,
                         const float Normalize_w, const float Normalize_h)
{
      int2 coordinate = (int2)(get_global_id(0), get_global_id(1));
      float2 normalizedCoordinate = convert_float2(coordinate) * (float2)(Normalize_w, Normalize_h);
      float4 colour = read_imagef(src_image, sampler, normalizedCoordinate);
      write_imagef(dst_image, coordinate, colour);
} 

