#pragma once

#include "common.h"
#include "formula.h"
#include <optional>
#include <iostream>
#include <set>

class Sheet;

/*Ваиант Ячейки Формула или текст*/
class CellVariant
{
public:
   using Value = CellInterface::Value;
   virtual Value GetValue() const = 0;
   virtual std::string GetText() const = 0;
   virtual std::vector<Position> GetReferencedCells() const = 0;
   virtual void ShowDiagnostic() const = 0;
   virtual ~CellVariant() = default;
};

class Cell : public CellInterface
{
public:
   enum class CellType
   {
      EMPTY,
      FORMULA,
      TEXT
   };

   Cell(Sheet &table);
   ~Cell();
   void Set(std::string text);
   void Clear();
   Value GetValue() const override;
   std::string GetText() const override;

   // оставляю метод только из-за интерфейса!!!
   // каждый раз конструировать вектор чтоб по нему идти
   // расточительно
   //  _GetReferencingCells()  со слешем впереди названия похожи - может сливаться
   std::vector<Position> GetReferencedCells() const override;
   const std::vector<Position> &GetReferencedCellsLink() const;
   std::set<Position> &_GetReferencingCells();
   const std::set<Position> &_GetReferencingCells() const;
   bool HasCache() { return (cache_ != std::nullopt); }
   void DestroyCache() { cache_.reset(); }
   void PrintDiagnostic() const;
   CellType GetType() const { return type_; }

private:
   CellType type_ = CellType::EMPTY;
   std::set<Position> from_me_depended_;
   CellVariant *cell_ = nullptr;
   std::vector<Position> depences_;
   Sheet &table_;
   mutable std::optional<Value> cache_;
};