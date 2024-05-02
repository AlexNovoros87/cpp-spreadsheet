#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <forward_list>
#include <map>
#include <set>
#include <queue>

class Sheet : public SheetInterface
{
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;
    const Cell *GetCell(Position pos) const override;
    Cell *GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;
    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;
    bool ExistsPosition(Position pos) const;
    bool HasCircularDepends(const Position &pos, const std::vector<Position> &dependes_list) const;
    void PrintDiagnostic() const;

private:
    void DeleteCache(const Position &pos);
    void DeleteCache(const std::set<Position> &deps_, std::set<Position> &visited);

    struct Hasher
    {
        size_t operator()(const Position &pos) const;
    };

    std::unordered_map<Position, Cell *, Hasher> table_;
    Size print_max_ = {-1, -1};
    const char tab = '\t';
};
