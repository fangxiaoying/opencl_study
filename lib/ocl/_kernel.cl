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