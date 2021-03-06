#include "DataView.h"
#include "ui_DataView.h"

#include "DataMatrixModel.h"
#include <qpushbutton.h>

DataView::DataView(QWidget *parent, const QString& samples)
	: QDialog(parent),
      ui(new Ui::DataView)
{
    ui->setupUi(this);
    ui->matrixView->showGrid();

	setWindowTitle("Data contained in " + samples);

	model_ = new DataMatrixModel(samples, this);
	ui->matrixView->setModel(model_);

	selectAllColumns_();

	selectAll_ = new QPushButton(this);
	selectAll_->setIcon(QIcon::fromTheme("edit-select-all"));
	selectAll_->setText(tr("Select all"));
	connect(selectAll_, SIGNAL(clicked()), this, SLOT(selectAllColumns_()));
	ui->buttons->addButton(selectAll_, QDialogButtonBox::ActionRole);

	deselectAll_ = new QPushButton(this);
	deselectAll_->setIcon(QIcon::fromTheme("edit-delete"));
	deselectAll_->setText(tr("Deselect all"));
	connect(deselectAll_, SIGNAL(clicked()), this, SLOT(deselectAllColumns_()));
	ui->buttons->addButton(deselectAll_, QDialogButtonBox::ActionRole);
    ui->matrixView->adjustSize();
    adjustSize();
}

void DataView::selectAllColumns_()
{
	const int columnCount = model_->columnCount(QModelIndex());
	QItemSelection selection(model_->index(0,0), model_->index(0, columnCount - 1));
	ui->matrixView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Columns);
}

void DataView::deselectAllColumns_()
{
	const int columnCount = model_->columnCount(QModelIndex());
	QItemSelection selection(model_->index(0,0), model_->index(0, columnCount - 1));
	ui->matrixView->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
}

DataView::~DataView()
{
	delete ui;
}

void DataView::accept()
{
	emit dataAccepted(model_->getSampleFile(), getDeselectedSamples());
	QDialog::accept();
}

const std::vector<unsigned int> DataView::getDeselectedSamples() const {
	std::vector<unsigned int> deselected;

	const int columnCount = model_->columnCount(QModelIndex());

	for(int i = 0; i < columnCount; ++i) {
		if(!ui->matrixView->selectionModel()->isColumnSelected(i, QModelIndex())) {
			deselected.push_back(i);
		}
	}

	return deselected;
}

QSize DataView::sizeHint() const{
    return QSize(500,model_->rowCount(QModelIndex())*50);
}
