/*
 * GUARD the Bridge
 * Copyright (C) 2012  Matthew Finkel <Matthew.Finkel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef gtbexceptions_h
#define gtbexceptions_h
#include <exception>

class PatronException : public std::exception
{
  public:
    PatronException(std::string m="Patron Exception caught!") : msg(m) {}
    ~PatronException() throw() {}
    const char* what() const throw() { return msg.c_str(); }

  private:
    std::string msg;
};

class BadConnectionException: public std::exception
{
  private:
    std::string m_where;
  public:
    BadConnectionException(std::string where)
    {
      m_where = where;
    }
    ~BadConnectionException() throw() {}
    virtual const char* what() const throw()
    {
      std::string what = "BadConnectionException at " + m_where;
      return what.c_str();
    }
};


#endif //gtbexceptions_h
