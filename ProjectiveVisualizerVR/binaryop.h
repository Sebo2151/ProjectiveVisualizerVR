/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef BINARYOP_H
#define BINARYOP_H

#include "term.h"

class BinaryOp : public Term
{
public:
    enum op_type { OP_PLUS, OP_TIMES, OP_MINUS, OP_EXP };

    BinaryOp(op_type op, Term* lhs, Term* rhs);

    virtual ~BinaryOp()
    {
        if (lhs) delete lhs;
        if (rhs) delete rhs;
        lhs = 0;
        rhs = 0;
    };

    virtual double eval(double x, double y, double z, double w);
    virtual Term* derivative(char var);
    virtual Term* Clone();
    virtual void print();
    static int op_priority(op_type op);
    virtual Term* simplify();
    virtual bool isNumerical() { return lhs->isNumerical() && rhs->isNumerical(); }
    virtual Term* homogenize(int* degree);
private:
    op_type op;
    Term* lhs;
    Term* rhs;
};

#endif // BINARYOP_H
