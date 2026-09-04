#include <shard/compilation/FrameLayoutPass.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

using namespace shard;

namespace
{
    struct Instr
    {
        std::size_t Offset = 0;
        std::size_t Next = 0;
        OpCode Op = OpCode::NOP;

        std::uint16_t Slot = 0;                     // LOAD/STORE_LOCAL/ARG slot, LOAD_TYPEARGUMENT index
        std::uint32_t Count = 0;                    // JUMP_TABLE target count
        std::size_t Target = 0;                     // JUMP*/BR_NULL/ENTER_TRY/DEFER target, DEFER_DRAIN count
        std::vector<std::size_t> Targets;           // JUMP_TABLE targets

        TypeSymbol* Type = nullptr;                 // NEWOBJECT/NEWARRAY_DYNAMIC/CREATERANGE/ISINSTANCE/CAST*/CASTPRIMITIVE
        MethodSymbol* Method = nullptr;             // CALLMETHODSYMBOL/CALLINTERFACE
        ConstructorSymbol* Ctor = nullptr;          // NEWOBJECT
        ArrayTypeSymbol* Array = nullptr;           // NEWARRAY
        DelegateTypeSymbol* Delegate = nullptr;     // CALLDELEGATE/NEWDELEGATE
        FieldSymbol* Field = nullptr;               // LOADSTATICFIELD/LOADENUMFIELD/STORESTATICFIELD
    };

    struct StackEffect
    {
        std::size_t Pops = 0;
        std::size_t Pushes = 0;
        std::size_t Payload = 0; // inline payload of the pushed value (0 when Pushes == 0)
    };

    struct Block
    {
        std::size_t Start = 0;
        std::size_t End = 0;                        // exclusive; End == Start means the block is empty
        std::vector<std::size_t> Successors;        // offsets of successor block starts
        std::vector<std::size_t> FixedSuccessors;   // successors entered at a fixed depth (try handlers)
    };

    static bool IsTerminal(OpCode op)
    {
        return op == OpCode::RETURN
            || op == OpCode::THROW
            || op == OpCode::RETHROW
            || op == OpCode::HALT;
    }

    // Decodes the operand bytes of a single instruction.
    // Must stay in sync with :
    // * ByteCodeEncoder (operand emission),
    // * VirtualMachine::ProcessCode (operand consumption),
    // * ProgramDisassembler (operand printing).
    static bool DecodeInstr(ByteCodeDecoder& decoder, Instr& in)
    {
        switch (in.Op)
        {
            case OpCode::NOP:
            case OpCode::HALT:
            case OpCode::POP:
            case OpCode::LOADCONST_NULL:
            case OpCode::RETURN:
            case OpCode::DUP:
            case OpCode::MATH_ADDITION:
            case OpCode::MATH_SUBTRACT:
            case OpCode::MATH_MULTIPLY:
            case OpCode::MATH_DIVISION:
            case OpCode::MATH_MODULO:
            case OpCode::MATH_POWER:
            case OpCode::MATH_NEGATIVE:
            case OpCode::MATH_SHL:
            case OpCode::MATH_SHR:
            case OpCode::COMPARE_EQUAL:
            case OpCode::COMPARE_NOTEQUAL:
            case OpCode::COMPARE_LESS:
            case OpCode::COMPARE_LESS_OR_EQUAL:
            case OpCode::COMPARE_GREATER:
            case OpCode::COMPARE_GREATER_OR_EQUAL:
            case OpCode::LOGICAL_NOT:
            case OpCode::LOGICAL_OR:
            case OpCode::LOGICAL_AND:
            case OpCode::LOADARRAYELEMENT:
            case OpCode::STOREARRAYELEMENT:
            case OpCode::ARRAYLENGTH:
            case OpCode::LEAVE_TRY:
            case OpCode::RETHROW:
            case OpCode::END_CATCH:
            case OpCode::LOAD_CURRENT_EXCEPTION:
            case OpCode::STORE_CURRENT_EXCEPTION:
            case OpCode::DEFER_BREAK:
                break;

            case OpCode::LOADCONST_BOOLEAN:
                decoder.AbsorbBoolean();
                break;
            
            case OpCode::LOADCONST_INTEGER8:
                decoder.AbsorbUInt8();
                break;
            
            case OpCode::LOADCONST_INTEGER64:
                decoder.AbsorbInt64();
                break;
            
            case OpCode::LOADCONST_NATIVE_INTEGER:
                decoder.AbsorbIntPtr();
                break;
            
            case OpCode::LOADCONST_RATIONAL64:
                decoder.AbsorbDouble64();
                break;
            
            case OpCode::LOADCONST_CHAR:
                decoder.AbsorbChar16();
                break;
            
            case OpCode::LOADCONST_STRING:
                decoder.AbsorbString();
                break;

            case OpCode::LOAD_LOCAL:
            case OpCode::STORE_LOCAL:
            case OpCode::LOAD_ARG:
            case OpCode::STORE_ARG:
                in.Slot = decoder.AbsorbVariableSlot();
                break;

            case OpCode::JUMP:
            case OpCode::JUMP_FALSE:
            case OpCode::JUMP_TRUE:
            case OpCode::BR_NULL:
            case OpCode::ENTER_TRY:
            case OpCode::DEFER:
                in.Target = decoder.AbsorbJump();
                break;

            case OpCode::DEFER_DRAIN:
                in.Target = decoder.AbsorbJump(); // drain count, not a control-flow target
                break;

            case OpCode::JUMP_TABLE:
            {
                in.Count = decoder.AbsorbUInt32();
                in.Targets.resize(in.Count);
                for (std::uint32_t i = 0; i < in.Count; ++i)
                    in.Targets[i] = decoder.AbsorbJump();
                break;
            }

            case OpCode::CALLMETHODSYMBOL:
            case OpCode::CALLINTERFACE:
                in.Method = decoder.AbsorbMethodSymbol();
                break;

            case OpCode::CALLDELEGATE:
            case OpCode::NEWDELEGATE:
                in.Delegate = decoder.AbsordDelegateTypeSymbol();
                break;

            case OpCode::NEWOBJECT:
                in.Type = decoder.AbsorbTypeSymbol();
                in.Ctor = decoder.AbsorbConstructorSymbol();
                break;

            case OpCode::LOADFIELD:
            case OpCode::STOREFIELD:
                in.Count = decoder.AbsorbFieldSlot();
                break;

            case OpCode::LOADSTATICFIELD:
            case OpCode::LOADENUMFIELD:
            case OpCode::STORESTATICFIELD:
                in.Field = decoder.AbsorbFieldSymbol();
                break;

            case OpCode::NEWARRAY:
                in.Array = decoder.AbsorbArraySymbol();
                break;

            case OpCode::NEWARRAY_DYNAMIC:
            case OpCode::CREATERANGE:
            case OpCode::ISINSTANCE:
            case OpCode::CAST_AS:
            case OpCode::CAST:
            case OpCode::CASTPRIMITIVE:
                in.Type = decoder.AbsorbTypeSymbol();
                break;

            case OpCode::LOAD_TYPEARGUMENT:
                in.Slot = decoder.AbsorbUInt16();
                decoder.AbsorbTypeSymbol();
                break;

            default:
                return false;
        }

        return true;
    }

    static std::size_t SlotPayload(const MethodSymbol& method, std::uint16_t slot)
    {
        const std::uint16_t argsCount = method.GetEvalStackArgumentsCount();

        if (slot < argsCount)
        {
            std::size_t paramIndex = slot;
            if (method.Linking == LINK_INSTANCE)
            {
                if (slot == 0)
                    return sizeof(void*); // implicit 'this' is a reference

                --paramIndex;
            }

            if (paramIndex < method.Parameters.size() && method.Parameters[paramIndex] != nullptr)
                return FrameLayout::ResolveTypePayload(const_cast<TypeSymbol*>(method.Parameters[paramIndex]->Type));

            return sizeof(void*);
        }

        const std::size_t variableIndex = static_cast<std::size_t>(slot) - argsCount;
        if (variableIndex < method.Layout.VariableSlots.size())
        {
            const FrameSlotRecipe& recipe = method.Layout.VariableSlots[variableIndex];
            if (recipe.TypeParameterIndex >= 0)
                return sizeof(void*); // unresolved generic parameter: reference-sized

            return FrameLayout::ResolveTypePayload(recipe.ConcreteType);
        }

        return sizeof(void*); // slot outside recorded layout: reference-sized
    }

    static bool ReturnsValue(const MethodSymbol* method)
    {
        return method != nullptr && method->ReturnType != nullptr &&
               method->ReturnType != SymbolTable::Primitives::Void;
    }

    static StackEffect CallEffect(const MethodSymbol* target)
    {
        if (target == nullptr)
            return { 1, 1, sizeof(void*) }; // unresolvable: under-model pops, reference push

        StackEffect fx;
        fx.Pops = target->GetEvalStackArgumentsCount();
        if (ReturnsValue(target))
        {
            fx.Pushes = 1;
            fx.Payload = FrameLayout::ResolveTypePayload(target->ReturnType);
        }

        return fx;
    }

    static StackEffect GetEffect(const MethodSymbol& method, const Instr& in)
    {
        switch (in.Op)
        {
            // no stack traffic
            case OpCode::NOP:
            case OpCode::HALT:
            case OpCode::LEAVE_TRY:
            case OpCode::END_CATCH:
            case OpCode::DEFER:
            case OpCode::DEFER_BREAK:
            case OpCode::DEFER_DRAIN:
            case OpCode::ENTER_TRY:
            case OpCode::LOAD_TYPEARGUMENT:
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_ARG:
            case OpCode::JUMP:
                return { 0, 0, 0 };

            // terminals
            case OpCode::RETURN:
            case OpCode::THROW:
            case OpCode::RETHROW:
                return { 0, 0, 0 };

            // constants
            case OpCode::LOADCONST_NULL:
                return { 0, 1, sizeof(void*) };
            
            case OpCode::LOADCONST_BOOLEAN:
                return { 0, 1, 1 };
            
            case OpCode::LOADCONST_INTEGER8:
                return { 0, 1, 1 };
            
            case OpCode::LOADCONST_INTEGER64:
                return { 0, 1, 8 };
            
            case OpCode::LOADCONST_NATIVE_INTEGER:
                return { 0, 1, sizeof(void*) };
            
            case OpCode::LOADCONST_RATIONAL64:
                return { 0, 1, 8 };
            
            case OpCode::LOADCONST_CHAR:
                return { 0, 1, 2 };
            
            case OpCode::LOADCONST_STRING:
                return { 0, 1, sizeof(void*) };

            // locals/args
            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_ARG:
                return { 0, 1, SlotPayload(method, in.Slot) };

            // conditional jumps
            case OpCode::JUMP_FALSE:
            case OpCode::JUMP_TRUE:
            case OpCode::BR_NULL:
            case OpCode::JUMP_TABLE:
                return { 1, 0, 0 };

            case OpCode::POP: return { 1, 0, 0 };

            case OpCode::DUP:
                return { 0, 1, sizeof(void*) }; // copies are reference-sized views

            // unary
            case OpCode::MATH_NEGATIVE:
            case OpCode::LOGICAL_NOT:
                return { 1, 1, sizeof(void*) };

            // binary
            case OpCode::MATH_ADDITION:
            case OpCode::MATH_SUBTRACT:
            case OpCode::MATH_MULTIPLY:
            case OpCode::MATH_DIVISION:
            case OpCode::MATH_MODULO:
            case OpCode::MATH_POWER:
            case OpCode::MATH_SHL:
            case OpCode::MATH_SHR:
            case OpCode::COMPARE_EQUAL:
            case OpCode::COMPARE_NOTEQUAL:
            case OpCode::COMPARE_LESS:
            case OpCode::COMPARE_LESS_OR_EQUAL:
            case OpCode::COMPARE_GREATER:
            case OpCode::COMPARE_GREATER_OR_EQUAL:
            case OpCode::LOGICAL_OR:
            case OpCode::LOGICAL_AND:
                return { 2, 1, sizeof(void*) };

            // calls
            case OpCode::CALLMETHODSYMBOL:
            case OpCode::CALLINTERFACE:
                return CallEffect(in.Method);

            case OpCode::CALLDELEGATE:
            {
                if (in.Delegate == nullptr || in.Delegate->AnonymousSymbol == nullptr)
                    return { 1, 1, sizeof(void*) };

                // The delegate instance is popped; instance (closure) targets get it
                // back as 'this' before the callee consumes the arguments, so the
                // net pops are the signature's argument count plus one receiver slot
                // for non-instance targets. Assigned targets with receivers pop more
                // at runtime, which only raises the real depth above the model.
                const MethodSymbol* anon = in.Delegate->AnonymousSymbol;
                StackEffect fx;
                fx.Pops = static_cast<std::size_t>(anon->GetEvalStackArgumentsCount()) + 1;
                
                if (ReturnsValue(anon))
                {
                    fx.Pushes = 1;
                    fx.Payload = FrameLayout::ResolveTypePayload(anon->ReturnType);
                }

                return fx;
            }

            // object construction
            case OpCode::NEWOBJECT:
            {
                StackEffect fx;
                fx.Pops = in.Ctor != nullptr ? in.Ctor->GetEvalStackArgumentsCount() : 1;
                fx.Pushes = 1;
                fx.Payload = in.Type != nullptr ? FrameLayout::ResolveTypePayload(in.Type) : sizeof(void*);
                return fx;
            }

            case OpCode::NEWDELEGATE:
            {
                StackEffect fx;
                fx.Pushes = 1;
                fx.Payload = in.Delegate != nullptr ? FrameLayout::ResolveTypePayload(in.Delegate) : sizeof(void*);
                if (in.Delegate != nullptr &&
                    in.Delegate->AnonymousSymbol != nullptr &&
                    in.Delegate->AnonymousSymbol->Linking == LINK_INSTANCE)
                {
                    fx.Pops = 1; // closure receiver already on the stack
                }

                return fx;
            }

            // fields
            case OpCode::LOADFIELD:
                return { 1, 1, sizeof(void*) }; // slot operand carries no type metadata; field views are references

            case OpCode::STOREFIELD:
                return { 2, 0, 0 };

            case OpCode::LOADSTATICFIELD:
                return { 0, 1, sizeof(void*) };

            case OpCode::STORESTATICFIELD:
                return { 1, 0, 0 };

            case OpCode::LOADENUMFIELD:
            {
                if (in.Field == nullptr)
                    return { 0, 1, sizeof(void*) };

                TypeSymbol* enumType = static_cast<TypeSymbol*>(in.Field->Parent);
                return { 0, 1, FrameLayout::ResolveTypePayload(enumType) };
            }

            // arrays
            case OpCode::NEWARRAY:
                return { in.Array != nullptr ? in.Array->Length : 1, 1, sizeof(void*) };

            case OpCode::NEWARRAY_DYNAMIC:
                return { 1, 1, sizeof(void*) };

            case OpCode::CREATERANGE:
                return { 3, 1, sizeof(void*) };

            case OpCode::LOADARRAYELEMENT:
                return { 2, 1, sizeof(void*) };

            case OpCode::STOREARRAYELEMENT:
                return { 3, 0, 0 };

            case OpCode::ARRAYLENGTH:
                return { 1, 1, 8 };

            // type checks and casts
            case OpCode::ISINSTANCE:
                return { 1, 1, 1 };

            case OpCode::CAST_AS:
            case OpCode::CAST:
                return { 1, 1, sizeof(void*) };

            case OpCode::CASTPRIMITIVE:
                return { 1, 1, in.Type != nullptr ? FrameLayout::ResolveTypePayload(in.Type) : sizeof(void*) };

            // exceptions
            case OpCode::LOAD_CURRENT_EXCEPTION:
                return { 0, 1, sizeof(void*) };

            case OpCode::STORE_CURRENT_EXCEPTION:
                return { 1, 0, 0 };
        }

        return { 0, 0, 0 };
    }
}

void FrameLayoutPass::Run(MethodSymbol& method)
{
    const std::vector<std::byte>& code = method.ExecutableByteCode;
    if (code.empty())
        return;

    // Pass 1: decode every instruction and collect jump targets.
    std::vector<Instr> instrs;
    std::set<std::size_t> targets;
    {
        ByteCodeDecoder decoder(code);
        while (!decoder.IsEOF())
        {
            Instr in;
            in.Offset = decoder.Index();
            in.Op = decoder.AbsorbOpCode();

            if (!DecodeInstr(decoder, in))
                return; // malformed bytecode: leave MaxEvalDepth at 0 (Stage 4 fallback)

            in.Next = decoder.Index();

            switch (in.Op)
            {
                case OpCode::JUMP:
                case OpCode::JUMP_FALSE:
                case OpCode::JUMP_TRUE:
                case OpCode::BR_NULL:
                case OpCode::ENTER_TRY:
                case OpCode::DEFER:
                {
                    targets.insert(in.Target);
                    break;
                }

                case OpCode::JUMP_TABLE:
                {
                    for (std::size_t target : in.Targets)
                        targets.insert(target);
                    break;
                }

                default:
                    break;
            }

            instrs.push_back(in);
        }
    }

    if (instrs.empty())
        return;

    // Pass 2: basic blocks. A block starts at offset 0, at every jump target,
    // and at the instruction following a terminal or an unconditional jump.
    std::set<std::size_t> starts;
    starts.insert(0);
    for (const Instr& in : instrs)
    {
        if (targets.count(in.Offset) > 0)
            starts.insert(in.Offset);

        if ((IsTerminal(in.Op) || in.Op == OpCode::JUMP) && in.Next < code.size())
            starts.insert(in.Next);
    }

    std::map<std::size_t, std::size_t> blockByStart;
    std::vector<Block> blocks;
    for (std::size_t start : starts)
    {
        blockByStart[start] = blocks.size();
        blocks.push_back(Block{ start, start });
    }

    for (const Instr& in : instrs)
    {
        // block containing in.Offset = last block start <= in.Offset
        std::size_t index = (--blockByStart.upper_bound(in.Offset))->second;
        blocks[index].End = in.Next;

        Block& block = blocks[index];
        auto addSuccessor = [&block](std::size_t offset)
        {
            if (std::find(block.Successors.begin(), block.Successors.end(), offset) == block.Successors.end())
                block.Successors.push_back(offset);
        };

        switch (in.Op)
        {
            case OpCode::JUMP:
            {
                addSuccessor(in.Target);
                break;
            }

            case OpCode::JUMP_FALSE:
            case OpCode::JUMP_TRUE:
            case OpCode::BR_NULL:
            {
                addSuccessor(in.Target);
                addSuccessor(in.Next);
                break;
            }

            case OpCode::JUMP_TABLE:
            {
                for (std::size_t target : in.Targets)
                    addSuccessor(target);
            
                // out-of-range index falls through the table
                addSuccessor(in.Next);
                break;
            }

            case OpCode::ENTER_TRY:
            {
                addSuccessor(in.Next);
                if (std::find(block.FixedSuccessors.begin(), block.FixedSuccessors.end(), in.Target) == block.FixedSuccessors.end())
                    block.FixedSuccessors.push_back(in.Target);
            
                break;
            }

            default:
            {
                if (!IsTerminal(in.Op))
                    addSuccessor(in.Next);
            
                break;
            }
        }
    }

    // Instruction lookup by offset for block simulation.
    std::map<std::size_t, std::size_t> instrByOffset;
    for (std::size_t i = 0; i < instrs.size(); ++i)
        instrByOffset[instrs[i].Offset] = i;

    // Pass 3: max-monotone dataflow over eval depths.
    std::vector<std::size_t> entryDepth(blocks.size(), 0);
    std::vector<bool> queued(blocks.size(), true);

    std::size_t maxDepth = 0;
    std::size_t maxPayload = 0;

    bool progressed = true;
    while (progressed)
    {
        progressed = false;
        for (std::size_t i = 0; i < blocks.size(); ++i)
        {
            if (!queued[i])
                continue;

            queued[i] = false;
            std::size_t depth = entryDepth[i];

            for (std::size_t ip = blocks[i].Start; ip < blocks[i].End;)
            {
                const Instr& in = instrs[instrByOffset[ip]];
                ip = in.Next;

                StackEffect fx = GetEffect(method, in);

                maxDepth = std::max(maxDepth, depth);
                depth = fx.Pops > depth ? 0 : depth - fx.Pops;
                depth += fx.Pushes;
                maxDepth = std::max(maxDepth, depth);

                if (fx.Pushes > 0)
                    maxPayload = std::max(maxPayload, fx.Payload);
            }

            auto propagate = [&](std::size_t start, std::size_t successorDepth)
            {
                auto found = blockByStart.find(start);
                if (found == blockByStart.end())
                    return;

                std::size_t successor = found->second;
                if (successorDepth > entryDepth[successor])
                {
                    entryDepth[successor] = successorDepth;
                    queued[successor] = true;
                    progressed = true;
                }
            };

            for (std::size_t successorStart : blocks[i].Successors)
                propagate(successorStart, depth);

            // try handlers are entered with the eval stack truncated to the
            // locals region plus the pushed exception: fixed entry depth 1.
            for (std::size_t successorStart : blocks[i].FixedSuccessors)
                propagate(successorStart, 1);
        }
    }

    method.Layout.MaxEvalDepth = static_cast<std::uint32_t>(maxDepth);
    method.Layout.EvalSlotPayload = maxDepth > 0 ? std::max<std::size_t>(maxPayload, 1) : 0;
}
