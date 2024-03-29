//
// Filter
//
// Parse and apply user-defined filters to all the data.
// Filters are broadly just typed strings that restrict the data, such as:
//  - Hit properties ("hasProp("score") && prop("score") > 0.5")
//  - MC particle properties ("pdg("13")")
//  - Hit types ("view("U"))

const currentFilters = [];
let filterActive = false;
let previousFilter = "";
const filterTest = document.getElementById("filter_test");

export class UserFilter {
  constructor(state) {
    // Store the renderer state.
    this.state = state;

    // Internal state for the filter.
    this.currentFilters = [];
    this.currentString = "";

    this.filterElem = document.getElementById("filter_test");

    // Add event listener to the filter input.
    this.filterElem.addEventListener("input", () => {
      this.updateFilter();
    });

    // Using Ohm.js to parse the filter string.
    // Define a basic grammar for the filter.
    this.grammar = ohm.grammar(
    `Filter {
        Exp = Exp "&&" Exp -- and
            | Exp "||" Exp -- or
            | "(" Exp ")" -- parens
            | "!" Exp -- not
            | "hasProp(" Str ")" -- prop
            | "prop(" Str ")" Comp DoubleValue -- propComp
            | "pdg(" IntValue ")" -- pdg
            | "view(" View ")" -- view
        Str = letter+
        View = "U" | "V" | "W" | "u" | "v" | "w"
        Comp = "<" | "<=" | "==" | "!=" | ">=" | ">"
        IntValue = digit+
        DoubleValue = digit+ "."* digit*
    }`);

    // Now create a semantics object to interpret the grammar.
    this.semantics = this.grammar.createSemantics();
    this.semantics.addOperation("match", {
        Exp_and: (exp1, _, exp2) => {
            console.log("Checking and...");
            return exp1.match() && exp2.match();
        },
        Exp_or: (exp1, _, exp2) => {
            console.log("Checking or...");
            return exp1.match() || exp2.match();
        },
        Exp_parens: (_, exp, __) => {
            return exp.match();
        },
        Exp_not: (_, exp) => {
            console.log("Checking not...");
            return !exp.match();
        },
        Exp_prop: (_, str, __) => {
            console.log("Checking hasProp:", str.sourceString);
            // return this.state.activeHits.some((hit) => hit.properties.has(str.sourceString));
        },
        Exp_propComp: (_, propStr, __, comp, value) => {
            console.log("Checking prop:", propStr, comp, value);
            // return this.state.activeHits.some((hit) => {
            // if (!hit.properties.has(propStr.sourceString)) return false;
            // const prop = hit.properties.get(propStr.sourceString);
            // switch (comp.sourceString) {
            //     case "<":
            //     return prop < value.match();
            //     case "<=":
            //     return prop <= value.match();
            //     case "==":
            //     return prop === value.match();
            //     case "!=":
            //     return prop !== value.match();
            //     case ">=":
            //     return prop >= value.match();
            //     case ">":
            //     return prop > value.match();
            // }
            // });
        },
        Exp_pdg: (_, value, __) => {
            console.log("Checking PDG:", value.sourceString);
            // return this.state.activeHits.some((hit) => hit.pdg === value.match());
        },
        Exp_view: (_, value, __) => {
            console.log("Checking view:", value.sourceString);
            // return this.state.activeHits.some((hit) => hit.view === value.sourceString);
            return true;
        },
        Str: (chars) => {
            return chars.sourceString;
        },
        IntValue: (digits) => {
            return parseInt(digits.sourceString, 10);
        },
        DoubleValue: (before, _, after) => {
            return parseFloat(`${before.sourceString}.${after.sourceString}`);
        }
        });
  }

  updateFilter() {

    if (! this.state.visible) return;

    // Get the current filter string.
    const filterString = this.filterElem.value;

    // If the filter string is the same as the previous one, don't update.
    if (filterString === this.currentString) return;

    console.log("Updating filter to:", filterString);
    this.currentString = filterString;
    const matchResult = this.grammar.match(filterString);

    if (matchResult.failed()) {
      return;
    }

    this.semantics(matchResult).match();
  }
}
