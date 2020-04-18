/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 10.0
  error: ""
  filename: "parameter/parameter_list"
END_TEST_DATA
*/

struct Test
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;


parameter::list<ParameterType, ParameterType> pc;

container::chain<Test, Test> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	pc.get<0>().connect(first);
	pc.get<1>().connect(second);

	pc.call<0>(2.0);
	pc.call<1>(8.0);
	
	return c.get<0>().value + c.get<1>().value;
}


