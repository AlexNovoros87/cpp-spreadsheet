#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <numeric>
#include <cmath>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe)
{
    return output << "#ARITHM!";
}

namespace
{
    class Formula : public FormulaInterface
    {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression)
        try : ast_(ParseFormulaAST(expression)),
            exp_(std::move(expression))
        {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            expn_ = std::move(ss.str());
        }
        catch (...)
        {
            throw FormulaException("Parsing Error");
        }
        Value Evaluate(const SheetInterface &table) const override
        {

            try
            {
                double tempval = ast_.Execute(table);
                if (std::isinf(tempval) || std::isnan(tempval))
                    throw FormulaError(FormulaError::Category::Arithmetic);
                return Value(tempval);
            }
            catch (const FormulaError &fe)
            {
                return Value(FormulaError(fe.GetCategory()));
            }
            catch (...)
            {
                throw(FormulaError{FormulaError::Category::Value});
            }
            return Value(0.);
        }
        std::string GetExpression() const override
        {
            return expn_;
        }

        std::vector<Position> GetReferencedCells() const override
        {
            std::vector<Position> pos_s;
            for (auto &&p : ast_.GetCells())
            {
                if (std::find(pos_s.begin(), pos_s.end(), p) == pos_s.end())
                {
                    pos_s.push_back(p);
                }
            }
            return pos_s;
        };

    private:
        FormulaAST ast_;
        std::string exp_;
        std::string expn_;
    };
} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression)
{
    return std::make_unique<Formula>(std::move(expression));
}