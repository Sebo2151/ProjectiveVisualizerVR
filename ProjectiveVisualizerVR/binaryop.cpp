/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "binaryop.h"

#include <iostream>
#include "numericalterm.h"
#include "variable.h"

BinaryOp::BinaryOp(op_type op, Term* lhs, Term* rhs)
{
    this->op = op;
    this->lhs = lhs;
    this->rhs = rhs;
}

double BinaryOp::eval(double x, double y, double z, double w)
{
    double lhsval = lhs->eval(x,y,z,w);
    double rhsval = rhs->eval(x,y,z,w);

    switch (op)
    {
    case OP_PLUS:
        //std::cout << lhsval << " + " << rhsval << std::endl;
        return lhsval + rhsval;
    case OP_MINUS:
        //std::cout << lhsval << " - " << rhsval << std::endl;
        return lhsval - rhsval;
    case OP_TIMES:
        //std::cout << lhsval << " * " << rhsval << std::endl;
        return lhsval * rhsval;
    case OP_EXP:
        //std::cout << lhsval << " ^ " << rhsval << std::endl;
        return pow(lhsval,rhsval);
    default:
        throw BadTermException();
    }
}

Term* BinaryOp::derivative(char var)
{
    switch (op)
    {
    case OP_PLUS:
        return new BinaryOp(OP_PLUS, lhs->derivative(var), rhs->derivative(var));
    case OP_MINUS:
        return new BinaryOp(OP_MINUS, lhs->derivative(var), rhs->derivative(var));
    case OP_TIMES:
        // Product rule!
    {
        Term* rhs_clone = rhs->Clone();
        Term* lhs_clone = lhs->Clone();

        Term* left_prod = new BinaryOp(OP_TIMES, lhs->derivative(var), rhs_clone);
        Term* right_prod = new BinaryOp(OP_TIMES, lhs_clone, rhs->derivative(var));

        return new BinaryOp(OP_PLUS, left_prod, right_prod);
    }
    case OP_EXP:
    {
        // Power rule!
        // Assume exponent is integral... otherwise this will not be the derivative.
        Term* rhs_clone1 = rhs->Clone();
        Term* rhs_clone2 = rhs->Clone();
        Term* lhs_clone = lhs->Clone();

        Term* exponent = new BinaryOp(OP_MINUS, rhs_clone1, new NumericalTerm(1));
        Term* raise_to_exp = new BinaryOp(OP_EXP, lhs_clone, exponent);
        Term* derivative_of_inside = lhs_clone->derivative(var);
        Term* partial_product = new BinaryOp(OP_TIMES, rhs_clone2, derivative_of_inside);
        return new BinaryOp(OP_TIMES, partial_product, raise_to_exp);
    }
    default:
        throw BadTermException();
    }
}

Term* BinaryOp::Clone()
{
    Term* lhs_clone = lhs->Clone();
    Term* rhs_clone = rhs->Clone();
    return new BinaryOp(op, lhs_clone, rhs_clone);
}

void BinaryOp::print()
{
    std::cout << "(";
    lhs->print();

    switch (op)
    {
    case OP_PLUS:
        std::cout << ") + (";
        break;
    case OP_MINUS:
        std::cout << ") - (";
        break;
    case OP_TIMES:
        std::cout << ")*(";
        break;
    case OP_EXP:
        std::cout << ")^(";
    }

    rhs->print();
    std::cout << ")";
}

int BinaryOp::op_priority(op_type op)
{
    switch (op)
    {
    case OP_PLUS:
    case OP_MINUS:
        return 1;
    case OP_TIMES:
        return 2;
    case OP_EXP:
        return 3;
    default:
        throw BadTermException();
    }
}

Term* BinaryOp::simplify()
{
    Term* lhs_simp = lhs->simplify();
    Term* rhs_simp = rhs->simplify();

    if (lhs_simp->isNumerical() && rhs_simp->isNumerical())
    {
        Term* result = new NumericalTerm((int)round(eval(0,0,0,0)));
        delete lhs_simp;
        delete rhs_simp;
        return result;
    }

    switch (op)
    {
    case OP_PLUS:
        if (lhs_simp->isZero())
        {
            delete lhs_simp;
            return rhs_simp;
        }

        if (rhs_simp->isZero())
        {
            delete rhs_simp;
            return lhs_simp;
        }
        break;
    case OP_MINUS:
        if (rhs_simp->isZero())
        {
            delete rhs_simp;
            return lhs_simp;
        }
        break;
    case OP_TIMES:
        if (lhs_simp->isZero())
        {
            delete lhs_simp;
            delete rhs_simp;
            return new NumericalTerm(0);
        }

        if (lhs_simp->isOne())
        {
            delete lhs_simp;
            return rhs_simp;
        }

        if (rhs_simp->isZero())
        {
            delete lhs_simp;
            delete rhs_simp;
            return new NumericalTerm(0);
        }

        if (rhs_simp->isOne())
        {
            delete rhs_simp;
            return lhs_simp;
        }
        break;
    case OP_EXP:
        if (rhs_simp->isOne())
        {
            delete rhs_simp;
            return lhs_simp;
        }

        if (rhs_simp->isZero())
        {
            delete rhs_simp;
            delete lhs_simp;
            return new NumericalTerm(1);
        }

        if (lhs_simp->isOne())
        {
            delete rhs_simp;
            delete lhs_simp;
            return new NumericalTerm(1);
        }

        if (lhs_simp->isZero())
        {
            delete rhs_simp;
            delete lhs_simp;
            return new NumericalTerm(0);
        }
    }

    return new BinaryOp(op, lhs_simp, rhs_simp);
}

Term* BinaryOp::homogenize(int* degree)
{
    int lhs_degree;
    int rhs_degree;
    Term* lhs_hommed = lhs->homogenize(&lhs_degree);
    Term* rhs_hommed = rhs->homogenize(&rhs_degree);

    switch (op)
    {
    case OP_PLUS:
    case OP_MINUS:
        if (lhs_degree > rhs_degree)
        {
            *degree = lhs_degree;

            if (lhs_degree - rhs_degree == 1)
            {
                Term* rhs_more_hommed = new BinaryOp(OP_TIMES, rhs_hommed, new Variable(Variable::VAR_W));
                return new BinaryOp(op, lhs_hommed, rhs_more_hommed);
            }
            else
            {
                Term* rhs_adjustment = new BinaryOp(OP_EXP, new Variable(Variable::VAR_W), new NumericalTerm(lhs_degree - rhs_degree));
                Term* rhs_more_hommed = new BinaryOp(OP_TIMES, rhs_hommed, rhs_adjustment);
                return new BinaryOp(op, lhs_hommed, rhs_more_hommed);
            }
        }
        else if (lhs_degree < rhs_degree)
        {
            *degree = rhs_degree;

            if (rhs_degree - lhs_degree == 1)
            {
                Term* lhs_more_hommed = new BinaryOp(OP_TIMES, lhs_hommed, new Variable(Variable::VAR_W));
                return new BinaryOp(op, lhs_more_hommed, rhs_hommed);
            }
            else
            {
                Term* lhs_adjustment = new BinaryOp(OP_EXP, new Variable(Variable::VAR_W), new NumericalTerm(rhs_degree - lhs_degree));
                Term* lhs_more_hommed = new BinaryOp(OP_TIMES, lhs_hommed, lhs_adjustment);
                return new BinaryOp(op, lhs_more_hommed, rhs_hommed);
            }
        }
        else
        {
            *degree = lhs_degree;
            return new BinaryOp(op, lhs_hommed, rhs_hommed);
        }
    case OP_TIMES:
        *degree = lhs_degree + rhs_degree;
        return new BinaryOp(OP_TIMES, lhs_hommed, rhs_hommed);
    case OP_EXP: // Relies on numerical exponents.
        int exponent_val = (int)round(rhs_hommed->eval(0,0,0,0));
        *degree = lhs_degree * exponent_val;
        return new BinaryOp(OP_EXP, lhs_hommed,rhs_hommed);
    }
    throw BadTermException();
}
