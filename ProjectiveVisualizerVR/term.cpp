/*
    Projective Curve Viewer
    Copyright (C) 2016  Sebastian Bozlee

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "term.h"

#include "numericalterm.h"
#include "variable.h"
#include "binaryop.h"
#include <iostream>
#include <vector>

bool is_num(char c)
{
    return '0' <= c && c <= '9';
}

bool is_var(char c)
{
    return c == 'x' || c == 'y' || c == 'z' || c == 'w';
}

bool is_op(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '^';
}

// Advances i to the next non-space character in input,
// or to the end of the string.
// Returns true if i is at a non-space character after function call,
// returns false if i is at the end of the string.
bool next_nonspace(std::string* input, int* i)
{
    while(*i < input->length())
    {
        char c = (*input)[*i];
        if (!isspace(c))
            return true;
        (*i) += 1;
    }
    return false;
}

Term* parse_atomic_term(std::string* input, int* i)
{
    if (!next_nonspace(input, i))
        throw BadTermException("Expected Term.");

    // Hack for unary minus.
    if ((*input)[*i] == '-')
    {
        *i += 1;
        return new BinaryOp(BinaryOp::OP_MINUS, new NumericalTerm(0), parse_atomic_term(input, i));
    }

    if (is_num((*input)[*i]))
    {

        int num_so_far = (*input)[*i] - '0';
        *i += 1;
        for (; *i < input->length() && is_num((*input)[*i]); (*i)++)
        {
            num_so_far = ((*input)[*i] - '0') + num_so_far*10;
        }
        return new NumericalTerm(num_so_far);
    }
    else if (is_var((*input)[*i]))
    {
        switch ((*input)[*i])
        {
        case 'x':
            (*i) += 1;
            return new Variable(Variable::VAR_X);
        case 'y':
            (*i) += 1;
            return new Variable(Variable::VAR_Y);
        case 'z':
            (*i) += 1;
            return new Variable(Variable::VAR_Z);
        case 'w':
            (*i) += 1;
            return new Variable(Variable::VAR_W);
        }
    }
    else if ((*input)[*i] == '(')
    {
        int paren_count = 1;
        for (int j = *i + 1; j < input->length(); j++)
        {
            if ((*input)[j] == '(')
                paren_count += 1;
            else if ((*input)[j] == ')')
            {
                paren_count -= 1;
                if (paren_count == 0)
                {
                    std::string between_parens;
                    between_parens = input->substr(*i + 1, j - *i - 1);
                    *i = j + 1;
                    Term* parened_term = Term::parseTerm(between_parens);
                    return parened_term;
                }
                else if (paren_count < 0)
                    throw BadTermException("Too many closing parentheses.");
            }
        }
        throw BadTermException("Unclosed parenthesis.");

    }
    throw BadTermException("Unexpected character.");
}

BinaryOp::op_type get_op(std::string* input, int* i)
{
    if (!next_nonspace(input, i))
        throw BadTermException("Expected operator.");

    char c = (*input)[*i];
    if (c == '+')
    {
        (*i) += 1;
        return BinaryOp::OP_PLUS;
    }
    else if (c == '-')
    {
        (*i) += 1;
        return BinaryOp::OP_MINUS;
    }
    else if (c == '*')
    {
        (*i) += 1;
        return BinaryOp::OP_TIMES;
    }
    else if (is_num(c) || is_var(c) || c == '(')
        return BinaryOp::OP_TIMES;
    else if (c == '^')
    {
        (*i) += 1;
        return BinaryOp::OP_EXP;
    }

    throw BadTermException("Expected operator.");
}

void tokenize(std::string* input, std::vector<Term*>* terms, std::vector<BinaryOp::op_type>* ops)
{
    int i = 0;

    for (;;)
    {
        Term* next_term = parse_atomic_term(input, &i);
        terms->push_back(next_term);
        if (!next_nonspace(input, &i)) // If final term, leave.
            return;
        BinaryOp::op_type next_op = get_op(input, &i);
        ops->push_back(next_op);
    }
}

void build_exponents(std::vector<Term*>* terms, std::vector<BinaryOp::op_type>* ops)
{
    // Right to left for right associativity.
    for (int i = ops->size() - 1; i >= 0; i--)
    {
        if ((*ops)[i] == BinaryOp::OP_EXP)
        {
            if (!(*terms)[i + 1]->isNumerical())
            {
                throw BadTermException("Variables not allowed in exponents.");
            }

            if ((*terms)[i + 1]->eval(0,0,0,0) < 0)
            {
                throw BadTermException("Only positive exponents allowed.");
            }

            Term* rhs = (*terms)[i + 1];
            Term* lhs = (*terms)[i];

            Term* combined = new BinaryOp(BinaryOp::OP_EXP, lhs, rhs);

            terms->erase(terms->begin() + i, terms->begin() + i + 2);
            terms->insert(terms->begin() + i, combined);
            ops->erase(ops->begin() + i);
        }
    }
}

void build_multiplication(std::vector<Term*>* terms, std::vector<BinaryOp::op_type>* ops)
{
    // Left to right for left associativity.
    for (int i = 0; i < ops->size();)
    {
        if ((*ops)[i] == BinaryOp::OP_TIMES)
        {
            Term* rhs = (*terms)[i + 1];
            Term* lhs = (*terms)[i];

            Term* combined = new BinaryOp(BinaryOp::OP_TIMES, lhs, rhs);

            terms->erase(terms->begin() + i, terms->begin() + i + 2);
            terms->insert(terms->begin() + i, combined);
            ops->erase(ops->begin() + i);
        }
        else
            i++;
    }
}

void build_addition_subtraction(std::vector<Term*>* terms, std::vector<BinaryOp::op_type>* ops)
{
    // Left to right for left associativity.
    for (int i = 0; i < ops->size();)
    {
        if ((*ops)[i] == BinaryOp::OP_PLUS || (*ops)[i] == BinaryOp::OP_MINUS)
        {
            Term* rhs = (*terms)[i + 1];
            Term* lhs = (*terms)[i];

            Term* combined = new BinaryOp((*ops)[i], lhs, rhs);

            terms->erase(terms->begin() + i, terms->begin() + i + 2);
            terms->insert(terms->begin() + i, combined);
            ops->erase(ops->begin() + i);
        }
        else
            i++;
    }
}

Term* Term::parseTerm(std::string input)
{
    std::vector<Term*> terms;
    std::vector<BinaryOp::op_type> ops;

    tokenize(&input, &terms, &ops); // Handles parentheses.

    build_exponents(&terms, &ops);
    build_multiplication(&terms, &ops);
    // No division allowed.
    build_addition_subtraction(&terms, &ops);

    if (terms.size() != 1)
        throw BadTermException("Parser error.");

    return terms[0];
}
