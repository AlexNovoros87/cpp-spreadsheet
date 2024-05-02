#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class StringCell : public CellVariant
{
public:
   StringCell(std::string text) : txt_(std::move(text)) {}

   Value GetValue() const override
   {
      if (txt_[0] == '\'')
         return Value(txt_.substr(1));
      return Value(txt_);
   }
   std::string GetText() const override { return txt_; }
   std::vector<Position> GetReferencedCells() const override
   {
      return {};
   }

   void ShowDiagnostic() const override
   {
      std::cout << "i am text" << std::endl;
   }

private:
   std::string txt_;
};

class DoubleCell : public CellVariant
{
public:
   DoubleCell(std::string text, Sheet &table) : table_(table)
   {
      formula_ = ParseFormula(text.substr(1));
   }

   Value GetValue() const override
   {
      // This value = string -> double ->formila error
      // res - formulavalue double ->formila error
      auto res = formula_->Evaluate(reinterpret_cast<SheetInterface &>(table_));
      if (std::holds_alternative<double>(res))
         return Value(std::get<0>(res));
      return Value(std::get<1>(res));
   };

   std::string GetText() const override
   {
      return "=" + formula_->GetExpression();
   }

   std::vector<Position> GetReferencedCells() const override
   {
      return formula_->GetReferencedCells();
   }

   void ShowDiagnostic() const override
   {
      using namespace std;
      cout << "i am formula" << endl;
      cout << "MyParents:";
      if (!GetReferencedCells().empty())
      {
         for (auto &&ref : GetReferencedCells())
         {
            cout << ref.row << "->" << ref.col << endl;
         }
      }
      else
         cout << "none" << endl;
   }

private:
   std::unique_ptr<FormulaInterface> formula_;
   Sheet &table_;
};

// Реализуйте следующие методы
Cell::Cell(Sheet &table) : table_(table){};

Cell::~Cell() { delete cell_; }

void Cell::Set(std::string text)
{
   if (text.empty())
   {
      Clear();
      type_ = CellType::EMPTY;
   }
   else if (text == "=")
   {
      Clear();
      cell_ = new StringCell(text);
      type_ = CellType::TEXT;
   }
   else if (text[0] == '=')
   {
      Clear();
      cell_ = new DoubleCell(text, table_);
      type_ = CellType::FORMULA;
      depences_ = cell_->GetReferencedCells();
   }
   else
   {
      Clear();
      cell_ = new StringCell(text);
      type_ = CellType::TEXT;
   }
}

void Cell::Clear()
{
   delete cell_;
}

Cell::Value Cell::GetValue() const
{
   if (cache_.has_value())
      return cache_.value();
   if (cell_ != nullptr)
   {
      cache_ = Value(cell_->GetValue());
      return cache_.value();
   }
   return Value();
}

std::string Cell::GetText() const
{
   if (cell_)
   {
      return cell_->GetText();
   }
   return "";
}

void Cell::PrintDiagnostic() const
{
   using namespace std;
   if (cell_ == nullptr)
      cout << "I AM EMPTY" << endl;
   else
   {// show diagnostic cellvar
      auto v = GetValue();
      if (holds_alternative<double>(v))
         cout << "I AM DOUBLE:" << std::get<double>(v) << endl;
      else if (holds_alternative<std::string>(v))
         cout << "I AM STRING:" << std::get<std::string>(v) << endl;
      else
      {// else 1
         cout << "I AM ERROR :";
         switch (get<FormulaError>(v).GetCategory())
         { // sw
         case FormulaError::Category::Arithmetic:
            cout << "#ARITHM" << endl;
            break;
         case FormulaError::Category::Ref:
            cout << "#REF" << endl;
            break;
         case FormulaError::Category::Value:
            cout << "#VALUE" << endl;
            break;
         }// sw
      }// else 1
      cell_->ShowDiagnostic();
   }// show diagnostic cellvar

   cout << "FROM ME DEPENDED:";
   if (!from_me_depended_.empty())
   { // if
      for (auto &&dep : from_me_depended_)
      {// for
         cout << dep.row << "->" << dep.col << endl;
      }// for
   }// if
   else
      cout << "NONE" << endl;

   cout << "I DEPEND:";
   if (!depences_.empty())
   { // if
      for (auto &&dep : depences_)
      {// for
         cout << dep.row << "->" << dep.col << endl;
      }// for
   }// if
   else
      cout << "NONE" << endl;

   cout << "MY CACHE: ";
   if (cache_.has_value())
   { // if has valuse
      if (holds_alternative<double>(cache_.value()))
         cout << "I AM DOUBLE:" << std::get<double>(cache_.value()) << endl;
      else if (holds_alternative<std::string>(cache_.value()))
         cout << "I AM STRING:" << std::get<std::string>(cache_.value()) << endl;
      else
      { // if ferror
         cout << "I AM ERROR :";
         switch (get<FormulaError>(cache_.value()).GetCategory())
         { // switch
         case FormulaError::Category::Arithmetic:
            cout << "#ARITHM" << endl;
            break;
         case FormulaError::Category::Ref:
            cout << "#REF" << endl;
            break;
         case FormulaError::Category::Value:
            cout << "#VALUE" << endl;
            break;
         } // switch
      }    // if ferror
   }       // if has valuse
   else
      cout << "IS EMPTY" << endl;
} // end meth

std::vector<Position> Cell::GetReferencedCells() const
{
   return depences_;
};
const std::vector<Position> &Cell::GetReferencedCellsLink() const
{
   return depences_;
};

std::set<Position> &Cell::_GetReferencingCells()
{
   return from_me_depended_;
}
const std::set<Position> &Cell::_GetReferencingCells() const
{
   return from_me_depended_;
}