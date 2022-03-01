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

__kernel void zero_copy(__global const uchar3 *input_data,
                              __global uchar3 *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      
      int index = mad24(y, width, x);

      output_data[index] = input_data[index];

}

// __kernel void zero_copy(__global const uchar4 *input_data,
//                               __global uchar4 *output_data, const int  width)
// {

//       int x = get_global_id(0);
//       // int y = get_global_id(1);
      
//       // for(int i = 0; i < width; i++)
//             // output_data[x] = input_data[x];

// }

__kernel void BGRA2RGB(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);

      int in_index  =  mad24(y, width, x) * 4;
      int out_index =  mad24(y, width, x) * 3;

      output_data[out_index]      = input_data[in_index + 2];
      output_data[out_index + 1]  = input_data[in_index + 1];
      output_data[out_index + 2]  = input_data[in_index];
}

// __kernel void BGRA2RGB(__global const uchar *input_data,
//                               __global uchar *output_data, const int  width)
// {

//       int x = get_global_id(0);
//       int y = get_global_id(1);

//       // int i = get_local_id(0);
//       // int j = get_local_id(1);

//       // int RGB_index  = mad24(y, width, x);
//       // int BGRA_index = mad24(y, width, x) + RGB_index;

//       // output_data[RGB_index] = input_data[BGRA_index];


//       int index = mad24(y, width, x);

//       output_data[index] = input_data[index];



// //       output_data[out_index]      = input_data[in_index + 2];
// //       output_data[out_index + 1]  = input_data[in_index + 1];
// //       output_data[out_index + 2]  = input_data[in_index];
// }


__kernel void vector(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      int index = mad24(y, width, x);
      printf("%d ", index);

      // uchar4 BGRA = vload4(index, input_data);
      // vstore3(BGRA.xyz, index, output_data);

      // uchar4 BGRA = vload4(index, input_data);
      // vstore3(BGRA.xyz, index, output_data);

}