/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "variable.h"

#include <iostream>

#include "numericalterm.h"

Variable::Variable(var_type var)
{
    this->var = var;
}

double Variable::eval(double x, double y, double z, double w)
{
    switch (var)
    {
    case VAR_X:
        return x;
    case VAR_Y:
        return y;
    case VAR_Z:
        return z;
    case VAR_W:
        return w;
    default:
        throw BadTermException();
    }
}

Term* Variable::derivative(char var)
{
    switch (this->var)
    {
    case VAR_X:
        if (var == 'x')
            return new NumericalTerm(1);
        else
            return new NumericalTerm(0);
    case VAR_Y:
        if (var == 'y')
            return new NumericalTerm(1);
        else
            return new NumericalTerm(0);
    case VAR_Z:
        if (var == 'z')
            return new NumericalTerm(1);
        else
            return new NumericalTerm(0);
    case VAR_W:
        if (var == 'w')
            return new NumericalTerm(1);
        else
            return new NumericalTerm(0);
    default:
        throw BadTermException();
    }
}

void Variable::print()
{
    switch (var)
    {
    case VAR_X:
        std::cout << "x";
        return;
    case VAR_Y:
        std::cout << "y";
        return;
    case VAR_Z:
        std::cout << "z";
        return;
    case VAR_W:
        std::cout << "w";
        return;
    default:
        throw BadTermException();
    }
}

Term* Variable::Clone()
{
    return new Variable(var);
}
