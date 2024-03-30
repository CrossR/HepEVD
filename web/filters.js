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

    this.filterElem = document.getElementById("filter_test");
    this.filterCleanUp = () => {
      this.state.hitData.filterProps.clear();
      this.state.hitTypeState.activeTypesFilter.clear();
      this.filtering = false;
    };

    // Add event listener to the filter input.
    this.filterElem.addEventListener("input", () => {
      this.updateFilter();
    });

    // Using Ohm.js to parse the filter string.
    // Define a basic grammar for the filter.
    this.grammar = ohm.grammar(
      `Filter {
        Exp = "(" Exp ")" -- parens
            | Exp "&&" Exp -- and
            | Exp "||" Exp -- or
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
      Exp_parens: (_, exp, __) => {
        console.log("Parens:", exp.sourceString);
        return exp.match();
      },
      Exp_and: (exp1, _, exp2) => {
        console.log("And:", exp1.sourceString, exp2.sourceString);
        const exp1Match = exp1.match();
        const exp2Match = exp2.match();

        if (!exp1Match || !exp2Match) return false;

        // Otherwise, we need to merge the two results.
        const andFilter = exp1Match;

        exp1Match.type.forEach((type, i) => {
          exp2Match.type.forEach((type2, j) => {
            console.log(type, type2);
            if (type === type2)
              andFilter.args[i] = andFilter.args[i].concat(exp2Match.args[j]);
          });
        });

        return andFilter;
      },
      Exp_or: (exp1, _, exp2) => {
        console.log("Or:", exp1.sourceString, exp2.sourceString);
        const exp1Match = exp1.match();
        const exp2Match = exp2.match();

        if (!exp1Match && !exp2Match) return false;

        // Otherwise, we need to append the two results.
        const orFilter = {
          args: [...exp1Match.args, ...exp2Match.args],
          setter: [...exp1Match.setter, ...exp2Match.setter],
          type: [...exp1Match.type, ...exp2Match.type],
        };

        return orFilter;
      },
      Exp_prop: (_, str, __) => {
        const checkHitProp = (propNames, props) => {
          return propNames.every((prop) => props.has(prop));
        };
        const addHitProp = {
          args: [[str.sourceString]],
          setter: [
            (args) =>
              this.state.hitData.setHitProperty({
                args: args,
                func: checkHitProp,
              }),
          ],
          type: ["prop"],
        };

        return addHitProp;
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

        return true;
      },
      Str: (chars) => {
        console.log("String:", chars.sourceString);
        return chars.sourceString;
      },
      IntValue: (digits) => {
        console.log("Int:", digits.sourceString);
        return parseInt(digits.sourceString, 10);
      },
      DoubleValue: (before, _, after) => {
        console.log("Double:", before.sourceString, after.sourceString);
        return parseFloat(`${before.sourceString}.${after.sourceString}`);
      },
    });
  }

  updateFilter() {
    if (!this.state.visible) return;

    // Get the current filter string.
    const filterString = this.filterElem.value;

    if (filterString === undefined || filterString.trim() === "") {
      this.filterCleanUp();
      this.state.triggerEvent("fullUpdate");
      return;
    }

    const matchResult = this.grammar.match(filterString);

    // If the filter string doesn't match the grammar, finish.
    // However, we may still need to update the renderer, if there
    // was a previous filter applied.
    if (matchResult.failed() && this.filtering) {
      this.filterCleanUp();
      this.state.triggerEvent("fullUpdate");
      return;
    } else if (matchResult.failed()) {
      return;
    }

    // Run clean up for the previous filters, now that we have a new match.
    this.filterCleanUp();

    this.lastValidString = matchResult;
    const filterResult = this.semantics(matchResult).match();

    console.log(filterResult);

    if (filterResult.length === 0) {
      return;
    }

    // Actually apply the filters.
    const { setter, args } = filterResult;
    setter.forEach((set, i) => {
      set(args[i]);
    });

    // Update the renderer.
    this.state.triggerEvent("fullUpdate");
    this.filtering = true;
  }
}
