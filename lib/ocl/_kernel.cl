__kernel void BGRA2RGB(__global const uchar *input_data,
                              __global uchar *output_data, const int  width)
{

      int x = get_global_id(0);
      int y = get_global_id(1);
      int index = mad24(y, width, x);

      uchar4 BGRA = vload4(index, input_data);
      vstore3(BGRA.zyx, index, output_data);
}