"""A complete Python-authored schedule UI; windowless demo or --font interactive.

SOURCE CHECKPOINT: neither path has been run for this new binding yet.
"""
import argparse
from dataclasses import replace
from cui_bridge import (App, BridgeError, button, column, label, row, localized_time_field,
    TimeValue, TimeLocale, TimeFieldConfig, TimeCycle, TimeTextStatus)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', help='explicit existing TTF path for interactive SDL')
    args = parser.parse_args()
    english_config = TimeFieldConfig(minimum=TimeValue(8, 0), maximum=TimeValue(18, 0))
    arabic_config = TimeFieldConfig(TimeLocale(tuple('٠١٢٣٤٥٦٧٨٩'), am='ص', pm='م'),
        cycle=TimeCycle.H12, minimum=TimeValue(8, 0), maximum=TimeValue(18, 0),
        segment_names=('ساعة', 'دقيقة', 'ثانية', 'فترة'))
    english = localized_time_field('english', english_config, TimeValue(9, 30), 'English24h')
    arabic = localized_time_field('arabic', arabic_config, TimeValue(10, 15), 'Arabic12h')
    summary = label('Applied time changes appear here', key='summary')
    observed = []
    def changed(event):
        # Native step/render already returned. The copied value is already applied;
        # no acceptance call or reapplication through time_value is necessary.
        observed.append(event)
        summary.label(f'{event.previous} → {event.value}; frame status={event.frame_status}')
        print(summary._text)
    english.on_time_change(changed); arabic.on_time_change(changed)
    def readonly(_event):
        config = english.get_config()
        english.configure(replace(config, readonly=not config.readonly))
    disabled = False
    def toggle_disabled(_event):
        nonlocal disabled
        disabled = not disabled
        english.disabled(disabled)
    def bounds(_event):
        config = english.get_config()
        bounded = config.minimum is not None
        english.configure(replace(config, minimum=None if bounded else TimeValue(8, 0),
            maximum=None if bounded else TimeValue(18, 0), edit_generation=config.edit_generation + 1))
    def clear(_event):
        english.clear_validation(); arabic.clear_validation()
    controls = row('policy').gap(8).children(
        button('readonly').label('English read-only').on_click(readonly),
        button('disabled').label('English disabled').on_click(toggle_disabled))
    validation = row('validation').gap(8).children(
        button('bounds').label('Toggle08–18 bounds').on_click(bounds),
        button('clear').label('Clear validation').on_click(clear))
    options = dict(width=800, height=540, title='Python localized appointments')
    if args.font:
        options.update(backend='sdl', font_path=args.font)
    with App(**options) as app:
        app.enable_time_changes(16)
        app.mount(column('root').gap(8).children(label('Appointments', key='title'),
            row('english-row').children(english), row('arabic-row').children(arabic),
            controls, validation, summary))
        if args.font:
            app.run()  # Real host input and the same authored tree/dispatch.
            return
        app.step(); app.step()
        def paste(field, text):
            x, y, width, height = app.bounds(field)
            app.queue_pointer('down', x + width / 2, y + height / 2)
            app.queue_pointer('up', x + width / 2, y + height / 2)
            app.queue_text(text)
            try:
                app.step()
            except BridgeError:
                app.dispatch()  # Preserve applied observations after later failure.
                raise
            app.dispatch()
        paste(english, '11:45:30')
        paste(arabic, '٠١:٢٠:٠٠ م')
        paste(english, '23:00:00')
        assert english.get_value() == TimeValue(11, 45, 30)
        assert arabic.get_value() == TimeValue(13, 20)
        assert english.get_result().validation == TimeTextStatus.OUT_OF_RANGE
        assert len(observed) == 2
        clear(None); app.step()
        assert english.get_result().validation == TimeTextStatus.OK


if __name__ == '__main__':
    main()
