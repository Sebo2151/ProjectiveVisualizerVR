/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef TERM_H
#define TERM_H

#include "shared/Vectors.h"

class Term
{
public:
    Term(){}
    virtual ~Term() {}

    // Warning: allocates a new Term.
    static Term* parseTerm(std::string input);
    virtual double eval(double x, double y, double z, double w) = 0;
    double eval(Vector4 v) { return eval(v.x, v.y, v.z, v.w); }

    // Warning: allocates a new Term.
    virtual Term* derivative(char var) = 0;
    virtual void print() = 0;
    virtual Term* Clone() = 0;

    // Warning: allocates a new Term.
    virtual Term* simplify() { return Clone(); }
    // Warning: allocates a new Term.
    virtual Term* homogenize(int* degree) = 0;

    virtual bool isZero() { return false; }
    virtual bool isOne() { return false; }
    virtual bool isNumerical() { return false; }
};

class BadTermException
{
public:
    BadTermException(char* error_message = 0) { message = error_message; }
    char* getErrorMessage() { return message; }
private:
    char* message = 0;
};

#endif // TERM_H
