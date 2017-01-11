{% extends "search_html.tpl" %}

{% block ranklist %}
  <p>query answering time: {{exectime}}</p>
  {% for result in results %}
	{% set weight = result['weight'] %}
	{% set title = result['title'] %}
	{% set link = title.replace( " ", "_") %}
	<div id="search_rank">
	<div id="rank_weight">{{ "%.4f" % weight}}</div>
	<div id="rank_content">
	<div id="rank_title"><a href="http://en.wikipedia.org/wiki/{{link}}">{{title}}</a></div>
	</div>
	</div>
  {% end %}
{% end %}
