/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "numericalterm.h"

#include <iostream>

NumericalTerm::NumericalTerm(int val)
{
    this->val = val;
}

double NumericalTerm::eval(double x, double y, double z, double w)
{
    return val;
}

Term* NumericalTerm::derivative(char var)
{
    return new NumericalTerm(0);
}

Term* NumericalTerm::Clone()
{
    return new NumericalTerm(val);
}

void NumericalTerm::print()
{
    std::cout << val;
}
