"""Retained half-star review editor. Pass --font FONT.ttf for the native UI."""
import argparse
from cui_bridge import App, RatingConfig, button, column, label, rating

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font')
    args=parser.parse_args()
    status=label('Your score: 1.5')
    score=rating('score','Delivery').total(5).value(1.5)
    score.on_change(lambda event:status.label(f'Your score: {event.value:g}'))
    with App(backend='sdl' if args.font else 'none',font_path=args.font,width=460,height=260,title='Rate delivery') as app:
        supported=app.supports_rating_options()
        if supported:
            score.configure(RatingConfig(half_steps=True,hover_preview=True,step_down_filled=True,clearable=True,design_size='lg',width=360,fill_rgba=0xce4275ff,preview_rgba=0x245cb6ff))
        else:score.value(2)
        reset=button('Restore whole stars').on_click(lambda event:score.reset_config() if supported else score.value(0))
        app.mount(column('review').gap(12).children(label('Rate your delivery'),status,score,reset))
        if args.font:app.run()
        else:
            app.step()
            x,y,w,h=app.numeric_part_bounds(score,13)
            app.queue_pointer('down',x+w/4,y+h/2);app.queue_pointer('up',x+w/4,y+h/2);app.step();app.dispatch()
            assert score.current_value()==(3.5 if supported else 4)
            if supported:score.reset_config()
            assert score.get_config() is None
if __name__=='__main__':main()
