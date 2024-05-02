#include "sheet.h"
#include "cell.h"
#include "common.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <list>

using namespace std::literals;

inline std::ostream &operator<<(std::ostream &output, Position pos)
{
  return output << "(" << pos.row << ", " << pos.col << ")";
}

std::unique_ptr<SheetInterface> CreateSheet()
{
  return std::make_unique<Sheet>();
}

Sheet::~Sheet()
{
  for (auto &row : table_)
  {
    delete row.second;
  }
}

void Sheet::SetCell(Position pos, std::string text)
{
  if (!pos.IsValid())
    throw InvalidPositionException("SetCell");
  // Создаем временный объект
  Cell *temp = new Cell(*this);
  temp->Set(text);

  if (HasCircularDepends(pos, temp->GetReferencedCellsLink()))
  {
    delete temp;
    throw CircularDependencyException("Circular");
  }

  // Если ячейка есть по этому индексу - удаляем значение
  // сохраняя тех кто от нее ее зависит
  std::set<Position> old_depends;
  if (table_.count(pos) > 0)
  {
    old_depends = std::move(table_.at(pos)->_GetReferencingCells());
    delete table_.at(pos);
  }

  // Назначаем новое значение
  table_[pos] = temp;
  // возвращаем ей от кого она зависит
  (*table_.at(pos))._GetReferencingCells() = std::move(old_depends);

  // Генерируем новые ячейки которые встречаются в зависимостях
  // если их нет и говорим им что мы от них зависим
  for (auto &&dep : temp->GetReferencedCellsLink())
  {
    if (table_.count(dep) == 0)
    {
      table_[dep] = new Cell(*this);
    }
    table_.at(dep)->_GetReferencingCells().insert(pos);
  }

  // Удаляем кеш по всем зависящим от нас ячейкам
  for (auto &&p : table_.at(pos)->_GetReferencingCells())
  {
    DeleteCache(p);
  }

  // Проверка печатной области
  if (table_.at(pos)->GetType() != Cell::CellType::EMPTY)
  {
    if (print_max_.cols < pos.col)
      print_max_.cols = pos.col;
    if (print_max_.rows < pos.row)
      print_max_.rows = pos.row;
  }
}

bool Sheet::ExistsPosition(Position pos) const
{
  if (table_.count(pos) > 0)
  {
    return true;
  }
  return false;
};

const Cell *Sheet::GetCell(Position pos) const
{
  if (!pos.IsValid())
    throw InvalidPositionException("GetCell");
  if (ExistsPosition(pos))
    return table_.at(pos);
  return nullptr;
}

Cell *Sheet::GetCell(Position pos)
{
  if (!pos.IsValid())
    throw InvalidPositionException("GetCell");
  if (ExistsPosition(pos))
  {
    return table_.at(pos);
  }
  return nullptr;
}

void Sheet::ClearCell(Position pos)
{
  if (!pos.IsValid())
    throw InvalidPositionException("ClearCell");

  if (ExistsPosition(pos))
  {
    // Удаляем кеш
    DeleteCache(pos);
    // Удаляем объект
    delete table_.at(pos);
    table_.erase(pos);

    // Обнуляем область печати
    print_max_.cols = -1;
    print_max_.rows = -1;

    for (const auto &i : table_)
    {
      if (i.second->GetType() != Cell::CellType::EMPTY)
      {
        if (i.first.col > print_max_.cols)
          print_max_.cols = i.first.col;
        if (i.first.row > print_max_.rows)
          print_max_.rows = i.first.row;
      }
    }
  }
}

Size Sheet::GetPrintableSize() const
{
  Size limits = print_max_;
  return Size{limits.rows + 1, limits.cols + 1};
}

void Sheet::PrintValues(std::ostream &output) const
{
  Size limits = GetPrintableSize();
  for (int r = 0; r < limits.rows; ++r)
  {
    for (int c = 0; c < limits.cols; ++c)
    {
      Position t_pos{r, c};
      if (ExistsPosition(t_pos))
      {
        if (table_.at(t_pos)->GetType() != Cell::CellType::EMPTY)
        {
          const auto &printable = table_.at(t_pos)->GetValue();
          if (std::holds_alternative<std::string>(printable))
            output << std::get<0>(printable);
          else if (std::holds_alternative<double>(printable))
            output << std::get<1>(printable);
          else
            output << std::get<2>(printable).ToString() << "!";
        }
      }

      if (c < print_max_.cols)
        output << tab;
    }
    output << '\n';
  }
}

void Sheet::PrintTexts(std::ostream &output) const
{
  Size limits = GetPrintableSize();
  for (int r = 0; r < limits.rows; ++r)
  {
    for (int c = 0; c < limits.cols; ++c)
    {
      Position t_pos{r, c};
      if (ExistsPosition(t_pos))
      {
        if (table_.at(t_pos)->GetType() != Cell::CellType::EMPTY)
        {
          const std::string &pr = table_.at(t_pos)->GetText();
          output << pr;
        }
      }
      if (c < print_max_.cols)
        output << tab;
    }
    output << '\n';
  }
}

size_t Sheet::Hasher::operator()(const Position &pos) const
{
  size_t h1 = std::hash<int>{}(pos.col);
  size_t h2 = std::hash<int>{}(pos.row);
  return h1 * 5 + h2 * 7;
}

void Sheet::DeleteCache(const Position &pos)
{
  std::set<Position> visited;
  if (!pos.IsValid())
    return;
  if (table_.at(pos)->GetReferencedCellsLink().empty())
    return;
  DeleteCache(table_.at(pos)->_GetReferencingCells(), visited);
  table_.at(pos)->DestroyCache();
}

void Sheet::DeleteCache(const std::set<Position> &deps_, std::set<Position> &visited)
{
  for (auto &&i : deps_)
  {
    if (visited.count(i) > 0)
      continue;
    DeleteCache(table_.at(i)->_GetReferencingCells(), visited);
    table_.at(i)->DestroyCache();
    visited.insert(i);
  }
}

bool Sheet::HasCircularDepends(const Position &pos, const std::vector<Position> &dependes_list) const
{

  if (dependes_list.empty())
    return false;
  std::set<Position> storage;
  std::list<Position> queue_to_visit;

  // sheet->SetCell("M6"_pos, "=E2");
  for (const auto &depend : dependes_list)
  {                                   // for deplist
    queue_to_visit.push_back(depend); // 4 - 1
    const auto &front = queue_to_visit.front();

    while (!queue_to_visit.empty())
    { // while not empty
      if (table_.count(front) > 0)
      { // if depence count

        for (const auto &subdepence : table_.at(front)->GetReferencedCells())
        {// for subdepence
          if (storage.count(subdepence) == 0)
          { // if storage count
            queue_to_visit.push_back(subdepence);
            if (subdepence == pos)
              return true;
          } // if storage count
        }// for subdepence
      }// if depence count

      storage.insert(front);
      queue_to_visit.pop_front();
    } // while not empty
  }   // for deplist
  return (storage.count(pos) > 0);
};

void Sheet::PrintDiagnostic() const
{
  using namespace std;
  for (auto &&table_row : table_)
  {
    std::cout << endl
              << "Position(" << table_row.first.row << "->" << table_row.first.col << ") "
              << table_row.first.ToString() << endl;
    table_row.second->PrintDiagnostic();
  }
  cout << endl;
  cout << "--------------------------------------------------" << endl;
}