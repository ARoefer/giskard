#include "giskard/giskard_parser.hpp"

using namespace std;

int main(int argc, char const *argv[])
{
	giskard::Scope scope;
	string scal = "6.764";
	string vec = "(0.1, 0.5, 4.756)";
	bool t1 = giskard::parse_giskard(scal, scope);
	bool t2 = giskard::parse_giskard(vec, scope);


	cout << scal << " := " << t1 << endl;
	cout << vec  << " := " << t2 << endl;

	return 0;
}