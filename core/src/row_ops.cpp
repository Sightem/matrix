#include "matrix_core/row_ops.hpp"

#include "matrix_core/writer.hpp"

namespace matrix_core {

ErrorCode row_op_caption(const RowOp& op, char* out, std::size_t cap) noexcept {
		Writer w{out, cap, 0};

		switch (op.kind) {
		case RowOpKind::Swap: {
				w.append("$R_{");
				w.append_index1(op.target_row);
				w.append("} <-> R_{");
				w.append_index1(op.source_row);
				w.append("}$");
				return w.status();
		}
		case RowOpKind::Scale: {
				w.append("$R_{");
				w.append_index1(op.target_row);
				w.append("} \\leftarrow (");
				w.append_rational_latex(op.scalar);
				w.append(") R_{");
				w.append_index1(op.target_row);
				w.append("}$");
				return w.status();
		}
		case RowOpKind::AddMul: {
				w.append("$R_{");
				w.append_index1(op.target_row);
				w.append("} \\leftarrow R_{");
				w.append_index1(op.target_row);
				w.append("} + (");
				w.append_rational_latex(op.scalar);
				w.append(") R_{");
				w.append_index1(op.source_row);
				w.append("}$");
				return w.status();
		}
		}
		__builtin_unreachable();
}

} // namespace matrix_core
