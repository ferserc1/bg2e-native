"""Remove known disabled shader branches for source comparisons, without compilation."""
import re


def without_features(source, disabled):
    output, stack, active = [], [], True
    for line in source.splitlines(keepends=True):
        directive = line.strip()
        match = re.fullmatch(r'#if\s+(!?)(\w+)', directive)
        if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
            known = bool(match and match[2] in disabled)
            condition = bool(match[1]) if known else True
            stack.append((active, known, condition))
            if not known and active:
                output.append(line)
            active = active and condition
        elif directive == '#else':
            parent, known, condition = stack[-1]
            if known:
                active = parent and not condition
            elif parent:
                output.append(line)
        elif directive == '#endif':
            parent, known, _ = stack.pop()
            if not known and parent:
                output.append(line)
            active = parent
        elif active:
            output.append(line)
    assert not stack, 'Unbalanced shader preprocessor branches'
    return ''.join(output)
