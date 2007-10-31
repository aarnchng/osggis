/**
 * osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007 Glenn Waldron and Pelican Ventures, Inc.
 * http://osggis.org
 *
 * osgGIS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgGIS/ExpressionEvaluator>
#include <stack>

using namespace osgGIS;

/* -----------------------------------------------------------------------------
 * Copyright (c) 2003 Lallous <lallousx86@yahoo.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ----------------------------------------------------------------------------- 
 */

static const char *operator_mul   = "*";
static const char *operator_div   = "/";
static const char *operator_idiv  = "\\";
  static const char *operator_mod   = "%";
  static const char *operator_add   = "+";
  static const char *operator_sub   = "-";
  static const char *operator_xor   = "^";
  static const char *operator_band  = "&";
  static const char *operator_bor   = "|";
  static const char *operator_land  = "&&";
  static const char *operator_lor   = "||";
  static const char *operator_nor   = "!|";
  static const char *operator_nand  = "!&";
  static const char *operator_iseq  = "==";
  static const char *operator_ne    = "!=";
  static const char *operator_shl   = "<<";
  static const char *operator_shr   = ">>";
  static const char *operator_lt    = "<";
  static const char *operator_gt    = ">";
  static const char *operator_gte   = ">=";
  static const char *operator_lte   = "<=";

  struct operator_t
  {
    const char *op;
    int  precedence;
  };

  enum
  {
    eval_ok = 0,
    eval_unbalanced,
    eval_invalidoperator,
    eval_invalidoperand,
    eval_evalerr
  };

  static const operator_t operators[] =
  {
    {"<>=*/%+-^&|\\", -1}, // Operators[0] is reserved for storing all symbols that can be used in operators
    {operator_mul, 19}, 
    {operator_div, 19}, {operator_idiv, 19}, 
    {operator_mod, 18},
    {operator_shl, 17}, {operator_shr, 17},
    {operator_sub, 16}, {operator_add, 16},
    {operator_xor, 15}, {operator_band, 15}, {operator_bor, 15}, {operator_nor, 15}, 
    {operator_land, 14}, {operator_lor, 14}, {operator_nand, 15},
    {operator_iseq, 13}, {operator_lt, 13}, {operator_gt, 13},
    {operator_gte, 13}, {operator_ne, 13}, {operator_lte, 13}
  };
  
  // Returns < 0 if 'op' is not an operator
  // Otherwise it returns the index of the operator in the 'operators' array
  // The index can be used to get the operator's precedence value.
  static int isOperator(std::string op)
  {
    int i = 0, oplen = 0;
    std::string s = operators[0].op;
    // scan as long as it is a valid operator
    // an operator might  have not just one symbol to represent it

    while (i<op.length())
    {
      if (s.find(op[i]) == s.npos)
        break;
      oplen++;
      i++;
    }

    // no valid symbol in operator!s
    if (!oplen)
      return -1;

    // identify what operator that is and return its index
    // 1 = highest , N = lowest
    // -1 = invalid operator
    s = op.substr(0, oplen);
    for (i=1; i<sizeof(operators)/sizeof(operator_t); i++)
    {
      if (s == operators[i].op)
        return i;
    }
    return -1;
  }

  // returns the operands length.
  // scans as long as the current value is an alphanumeric or a decimal seperator
  static int getToken(std::string str)
  {
    int i=0, tokenLen = 0;
    while ( (i<str.length()) && (isalnum(str[i]) || (str[i]=='.')))
    {
      tokenLen++;
      i++;
    }
    return tokenLen;
  }

  static int toRPN(std::string exp, std::string &rpn)
  {
    std::stack<std::string> st;
    std::string token, topToken;
    char token1;
    int  tokenLen, topPrecedence, idx, precedence;
    std::string SEP(" "), EMPTY("");

    rpn = "";

    for (int i=0; i<exp.length();i++)
    {
      token1 = exp[i];

      // skip white space
      if (isspace(token1))
        continue;

      // push left parenthesis
      else if (token1 == '(')
      {
        st.push(EMPTY+token1);
        continue;
      }

      // flush all stack till matching the left-parenthesis
      else if (token1 == ')')
      {
        for (;;)
        {
          // could not match left-parenthesis
          if (st.empty())
            return eval_unbalanced;

          topToken = st.top();
          st.pop();
          if (topToken == "(")
            break;
          rpn.append(SEP + topToken);
        }
        continue;
      }

      // is this an operator?
      idx = isOperator(exp.substr(i));
    
      // an operand
      if (idx < 0)
      {
        tokenLen = getToken(exp.substr(i));
        if (tokenLen == 0)
          return eval_invalidoperand;

        token = exp.substr(i, tokenLen);
        rpn.append(SEP + token);
        i += tokenLen - 1;
        continue;
      }

      // is an operator
      else
      {
        // expression is empty or last operand an operator
        if (rpn.empty() || (isOperator(token) > 0))
        {
          rpn.append(SEP + "0");
        }
        // get precedence
        precedence = operators[idx].precedence;
        topPrecedence = 0;

        // get current operator 
        tokenLen = strlen(operators[idx].op);
        token    = exp.substr(i, tokenLen);
        i += tokenLen - 1;
        for (;;)
        {
          // get top's precedence
          if (!st.empty())
          {
            topToken = st.top();
            idx = isOperator(topToken.c_str());
            if (idx < 0)
              topPrecedence = 1; // give a low priority if operator not ok!
            else
              topPrecedence = operators[idx].precedence;
          }

          if (st.empty() || st.top() == "(" ||
              precedence > topPrecedence
             )
          {
            st.push(token);
            break;
          }
          // operator has lower precedence then pop it
          else
          {
            st.pop();
            rpn.append(SEP + topToken);
          }
        }
        continue;
      }
    }

    for (;;)
    {
      if (st.empty())
        break;
      topToken = st.top();
      st.pop();
      if (topToken != "(")
        rpn.append(SEP + topToken);
      else
      {
        return eval_unbalanced;
      }
    }
    return eval_ok;
  }

 template <typename T>
    static  int evaluateRPN(std::string rpn, T &result)
  {
    std::stack<T> st;
    char token1;
    std::string token;
    T r, op1, op2;
    int idx, tokenLen;

    for (int i=0;i<rpn.length();i++)
    {
      token1 = rpn[i];
      if (isspace(token1))
        continue;

      idx = isOperator(rpn.substr(i));

      // not an operator
      if (idx < 0)
      {
        tokenLen = getToken(rpn.substr(i));
        token    = rpn.substr(i, tokenLen);
        char *errstr;
        double d = strtod(token.c_str(), &errstr);
        if (*errstr)
          return eval_invalidoperand;
        r = d;
        st.push(r);
        i += tokenLen - 1;
        continue;
      }
      // an operator
      else
      {
        // get operand1 and op2
        op2 = st.top(); st.pop();
        if (!st.empty())
        {
          op1 = st.top(); st.pop();
        } else
          return eval_unbalanced;

        tokenLen = strlen(operators[idx].op);
        token    = rpn.substr(i, tokenLen);
        i += tokenLen - 1;

        if (token == operator_mul)
          r = op1 * op2;
        else if (token == operator_idiv)
          r = (long) (op1 / op2);
        else if (token == operator_div)
          r = op1 / op2;
        else if (token == operator_mod)
          r = (long)op1 % (long)op2;
        else if (token == operator_add)
          r = op1 + op2;
        else if (token == operator_sub)
          r = op1 - op2;
        else if (token == operator_land)
          r = op1 && op2;
        else if (token == operator_band)
          r = (long)op1 & (long)op2;
        else if (token == operator_lor)
          r = op1 || op2;
        else if (token == operator_bor)
          r = (long)op1 | (long)op2;
        else if (token == operator_xor)
          r = (long)op1 ^ (long)op2;
        else if (token == operator_nor)
          r = ~((long)op1 | (long)op2);
        else if (token == operator_nand)
          r = ~((long)op1 & (long)op2);
        else if (token == operator_iseq)
          r = op1 == op2;
        else if (token == operator_ne)
          r = op1 != op2;
        else if (token == operator_shl)
          r = (long)op1 << (long)op2;
        else if (token == operator_shr)
          r = (long)op1 >> (long)op2;
        else if (token == operator_lt)
          r = op1 < op2;
        else if (token == operator_lte)
          r = op1 <= op2;
        else if (token == operator_gt)
          r = op1 > op2;
        else if (token == operator_gte)
          r = op1 >= op2;
        // push result
        st.push(r);
      }
    }
    result = st.top();
    st.pop();
    if (!st.empty())
      return eval_evalerr;
    return eval_ok;
  }

  template <typename T> static int calculate(std::string expr, T &r)
  {
      std::string rpn;
    int err = eval_evalerr; // unexpected error

    try
    {
      if ( (err = toRPN(expr, rpn)) != eval_ok)
        return err;
      err = evaluateRPN(rpn, r);
    }
    catch(...)
    {
    }
    return err;
  }


//========================================================


ExpressionEvaluator::ExpressionEvaluator()
{
    //NOP
}

bool
ExpressionEvaluator::eval( const std::string& expr, int& out_result )
{
    return calculate( expr, out_result );
}

bool
ExpressionEvaluator::eval( const std::string& expr, double& out_result )
{
    return calculate( expr, out_result );
}
