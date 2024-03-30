//
// Filter
//
// Parse and apply user-defined filters to all the data.
// Filters are broadly just typed strings that restrict the data, such as:
//  - Hit properties ("hasProp("score") && prop("score") > 0.5")
//  - MC particle properties ("pdg("13")")
//  - Hit types ("view("U"))

export class UserFilter {
  constructor(state) {
    // Store the renderer state.
    this.state = state;

    // Internal state for the filter.
    this.filtering = false;
    this.filterCleanUp = [];
    this.lastMatch = {};

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
            | Exp Invalid -- invalid
            | "hasProp(" Str ")" -- prop
            | "prop(" Str ")" Comp DoubleValue -- propComp
            | "pdg(" IntValue ")" -- pdg
            | "view(" View ")" -- view
        Str = letter+
        View = "U" | "V" | "W" | "u" | "v" | "w"
        Comp = "<" | "<=" | "==" | "!=" | ">=" | ">"
        IntValue = digit+
        DoubleValue = digit+ "."* digit*
        Invalid = any*
    }`,
    );

    // Now create a semantics object to interpret the grammar.
    this.semantics = this.grammar.createSemantics();
    this.semantics.addOperation("match", {
      Exp_invalid: (exp, rest) => {
        return exp.match();
        },
      Exp_and: (exp1, _, exp2) => {
        return exp1.match() && exp2.match();
      },
      Exp_or: (exp1, _, exp2) => {
        // Here, we can't use short-circuiting, because we need to run both
        // sides to update the renderer.
        return [exp1.match(), exp2.match()].some((x) => x);
      },
      Exp_parens: (_, exp, __) => {
        return exp.match();
      },
      Exp_prop: (_, str, __) => {
        this.state.hitData.setHitProperty(str.sourceString);
        this.filterCleanUp.push(() => {
            this.state.hitData.setHitProperty(str.sourceString, false);
        });

        return true;
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
        const view = `${value.sourceString.toUpperCase()} View`;
        this.state.hitTypeState.addHitType(view);
        this.filterCleanUp.push(() => {
            this.state.hitTypeState.addHitType(view, false);
        });

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
      },
    });
  }

  updateFilter() {
    if (!this.state.visible) return;

    // Get the current filter string.
    const filterString = this.filterElem.value;

    if (filterString === undefined || filterString.trim() === "") {
        this.filterCleanUp.forEach((f) => f());
        this.filterCleanUp = [];
        this.state.triggerEvent("fullUpdate");
        return;
    }

    const matchResult = this.grammar.match(filterString);

    // If the filter string doesn't match the grammar, finish.
    // However, we may still need to update the renderer, if there
    // was a previous filter applied.
    if (matchResult.failed()) {
        return;
    }

    // Run clean up for the previous filter, now that we have a new match.
    this.filterCleanUp.forEach((f) => f());
    this.filterCleanUp = [];

    this.lastValidString = matchResult;
    this.semantics(matchResult).match();

    // Update the renderer.
    this.state.triggerEvent("fullUpdate");
  }
}
