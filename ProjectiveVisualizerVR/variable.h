/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef VARIABLE_H
#define VARIABLE_H

#include "term.h"

class Variable : public Term
{
public:
    enum var_type { VAR_X, VAR_Y, VAR_Z, VAR_W };
    Variable(var_type var);
    virtual ~Variable() {};

    double virtual eval(double x, double y, double z, double w);
    virtual Term* derivative(char var);
    virtual void print();
    virtual Term* Clone();
    virtual Term* homogenize(int* degree) { *degree = 1;
                                            return Clone(); }
private:
    var_type var;
};

#endif // VARIABLE_H
