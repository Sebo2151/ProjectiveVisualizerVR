/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef NUMERICALTERM_H
#define NUMERICALTERM_H

#include "term.h"

class NumericalTerm : public Term
{
public:
    NumericalTerm(int val);

    virtual double eval(double x, double y, double z, double w);
    virtual Term* derivative(char var);
    virtual void print();
    virtual Term* Clone();

    virtual bool isZero() {return val == 0; }
    virtual bool isOne() {return val == 1; }
    virtual bool isNumerical() {return true; }
    virtual Term* homogenize(int* degree) { *degree = 0;
                                            return Clone(); }
private:
    int val;

};

#endif // NUMERICALTERM_H
