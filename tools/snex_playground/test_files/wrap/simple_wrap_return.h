/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "wrap/simple_wrap_return"
END_TEST_DATA
*/

span<int, 8>::wrapped data = { 13 };


int main(int input)
{
	return data;
}

